#include "pch.h"
#include "eBPF.h"
#include <ebpf_api.h>
#include <bpf/bpf.h>
#include <io.h>

bool LocalClose(int fd) {
	return _close(fd) == 0;
}

std::vector<BpfProgram> BpfSystem::EnumPrograms() {
	uint32_t id = 0;
	std::vector<BpfProgram> programs;
	programs.reserve(8);

	for (;;) {
		auto err = bpf_prog_get_next_id(id, &id);
		if (err)
			break;

		auto p = GetProgramById(id);
		if (!p)
			break;

		programs.push_back(std::move(*p));
	}
	return programs;
}

std::unique_ptr<BpfProgram> BpfSystem::GetProgramById(uint32_t id) {
	auto fd = bpf_prog_get_fd_by_id(id);
	if (fd < 0)
		return nullptr;

	bpf_prog_info info{};
	uint32_t size = sizeof(info);
	auto p = std::make_unique<BpfProgram>();
	info.nr_map_ids = 32;
	p->MapIds.resize(info.nr_map_ids);
	info.map_ids = (uintptr_t)p->MapIds.data();
	auto err = bpf_obj_get_info_by_fd(fd, &info, &size);
	if (err) {
		LocalClose(fd);
		return nullptr;
	}

	p->MapIds.resize(info.nr_map_ids);
	p->Id = info.id;
	p->Name = info.name;
	p->LinkCount = info.link_count;
	p->Type = (BpfProgramType)info.type;
	p->UuidType = info.type_uuid;
	p->MapCount = info.nr_map_ids;
	p->PinnedPathCount = info.pinned_path_count;

	ebpf_execution_type_t type;
	const char* filename, * section;
	err = ebpf_program_query_info(fd, &type, &filename, &section);
	LocalClose(fd);

	if (err == 0) {
		p->ExecutionType = (BpfExecutionType)type;
		p->Section = section;
		p->FileName = filename;
	}
	ebpf_free_string(filename);
	ebpf_free_string(section);

	return p;
}

std::vector<BpfMap> BpfSystem::EnumMaps() {
	uint32_t id = 0;
	std::vector<BpfMap> maps;
	maps.reserve(8);
	for (;;) {
		auto err = bpf_map_get_next_id(id, &id);
		if (err)
			break;

		auto fd = bpf_map_get_fd_by_id(id);
		if (fd < 0)
			break;

		bpf_map_info info{};
		uint32_t size = sizeof(info);
		err = bpf_obj_get_info_by_fd(fd, &info, &size);
		LocalClose(fd);

		if (err)
			break;

		BpfMap map;
		map.Name = info.name;
		map.Id = info.id;
		map.InnerMapId = info.inner_map_id;
		map.Flags = info.map_flags;
		map.Type = (BpfMapType)info.type;
		map.KeySize = info.key_size;
		map.ValueSize = info.value_size;
		map.MaxEntries = info.max_entries;
		map.PinnedPathCount = info.pinned_path_count;

		maps.push_back(std::move(map));
	}
	return maps;
}

std::vector<BpfLink> BpfSystem::EnumLinks() {
	uint32_t id = 0;
	std::vector<BpfLink> links;
	links.reserve(8);

	for (;;) {
		auto err = bpf_link_get_next_id(id, &id);
		if (err) break;

		auto fd = bpf_link_get_fd_by_id(id);
		if (fd < 0) break;

		bpf_link_info info{};
		uint32_t size = sizeof(info);
		err = bpf_obj_get_info_by_fd(fd, &info, &size);
		LocalClose(fd);
		if (err) break;

		BpfLink link;
		link.Id = info.id;
		link.Type = (BpfLinkType)info.type;
		link.CGroupId = info.cgroup.cgroup_id;
		link.AttachType = (BpfAttachType)info.attach_type;
		link.ProgramId = info.prog_id;
		link.ProgramTypeUuid = info.program_type_uuid;
		link.AttachTypeUuid = info.attach_type_uuid;

		links.push_back(std::move(link));
	}
	return links;
}

std::vector<BpfMapItem> BpfSystem::GetMapData(uint32_t id) {
	auto fd = bpf_map_get_fd_by_id(id);
	if (fd < 0)
		return {};

	bpf_map_info info{};
	uint32_t size = sizeof(info);
	auto err = bpf_obj_get_info_by_fd(fd, &info, &size);
	if (err)
		return {};

	void const* keystart = nullptr;
	std::vector<BpfMapItem> data;
	data.reserve(info.max_entries);
	auto key = std::vector<uint8_t>(info.key_size);
	auto value = std::vector<uint8_t>(info.value_size);

	uint32_t index = 0;
	for (;;) {
		err = bpf_map_get_next_key(fd, keystart, key.data());
		if (err)
			break;

		err = bpf_map_lookup_elem(fd, key.data(), value.data());
		if (err)
			break;

		keystart = key.data();
		BpfMapItem item{ index++, std::make_unique<uint8_t[]>(info.key_size), std::make_unique<uint8_t[]>(info.value_size) };
		memcpy(item.Key.get(), key.data(), key.size());
		memcpy(item.Value.get(), value.data(), value.size());
		data.push_back(std::move(item));
	}

	LocalClose(fd);
	return data;
}

std::vector<BpfProgramEx> BpfSystem::EnumProgramsInFile(const char* path, std::string* errMsg) {
	ebpf_api_program_info_t* info;
	const char* msg;
	if (ebpf_enumerate_programs(path, true, &info, &msg) < 0) {
		if (errMsg)
			*errMsg = msg;
		return {};
	}

	std::vector<BpfProgramEx> programs;
	auto start = info;
	while (info) {
		BpfProgramEx p;
		p.FileName = path;
		p.Name = info->program_name;
		p.SectionName = info->section_name;
		p.Data.resize(info->raw_data_size);
		memcpy(p.Data.data(), info->raw_data, info->raw_data_size);
		p.Type = info->program_type;
		p.ExpectedAttachType = info->expected_attach_type;
		p.OffsetInSection = (uint32_t)info->offset_in_section;
		auto stats = info->stats;
		while (stats) {
			BpfStat stat{ stats->key, stats->value };
			p.Stats.push_back(std::move(stat));
			stats = stats->next;
		}
		programs.push_back(std::move(p));
		info = info->next;
	}

	ebpf_free_programs(start);

	return programs;
}

std::string BpfSystem::DisassembleProgram(BpfProgramEx const& p) {
	const char* text = nullptr;
	ebpf_api_elf_disassemble_program(p.FileName.c_str(), p.SectionName.c_str(), p.Name.c_str(), &text, &text);
	std::string result(text);
	ebpf_free_string(text);

	return result;
}

const char* BpfSystem::GetProgramTypeName(GUID const& type) {
	return ebpf_get_program_type_name(&type);
}

const char* BpfSystem::GetAttachTypeName(GUID const& type) {
	return ebpf_get_attach_type_name(&type);
}

BpfObject BpfSystem::LoadProgram(BpfProgramEx const& p, BpfExecutionType execType) {
	auto object = bpf_object__open(p.FileName.c_str());
	if (!object)
		return nullptr;

	BpfObject obj(object);

	if (bpf_object__load(obj) != EBPF_SUCCESS)
		return nullptr;

	if (ebpf_object_set_execution_type(object, (ebpf_execution_type_t)execType) != EBPF_SUCCESS)
		return nullptr;

	return obj;
}

bool BpfMap::IsPerCpu() const {
	return Type == BpfMapType::LruPerCpuHash || Type == BpfMapType::PerCpuArray || Type == BpfMapType::PerCpuHash;
}

BpfObject::BpfObject(bpf_object* obj) : m_Object(obj) {
}

BpfObject::operator bpf_object* () const {
	return m_Object;
}

BpfObject::~BpfObject() {
	if (m_Object)
		bpf_object__close(m_Object);
}
