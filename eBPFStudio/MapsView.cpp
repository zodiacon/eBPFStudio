#include "pch.h"
#include "MapsView.h"
#include "StringHelper.h"
#include <SortHelper.h>
#include "resource.h"
#include "PinPathDlg.h"

//bool SortPtr(const void* p1, const void* p2, uint32_t size, bool asc) {
//	if (size <= sizeof(long long)) {
//		long long v1 = 0, v2 = 0;
//		memcpy(&v1, p1, size);
//		memcpy(&v2, p2, size);
//		return SortHelper::Sort(v1, v2, asc);
//	}
//
//	auto compare = memcmp(p1, p2, size);
//	return asc ? compare > 0 : compare < 0;
//}

CString CMapsView::GetColumnText(HWND hWnd, int row, int column) const {
	if (hWnd == m_MapDataList)
		return GetColumnTextMapData(row, column);

	auto& m = m_Maps[row];
	switch (static_cast<ColumnType>(GetColumnManager(m_MapList)->GetColumnTag(column))) {
		case ColumnType::Name: return CString(m.Name.c_str());
		case ColumnType::Id: return std::to_wstring(m.Id).c_str();
		case ColumnType::Type: return StringHelper::MapTypeToString(m.Type);
		case ColumnType::MaxEntries: return std::to_wstring(m.MaxEntries).c_str();
		case ColumnType::KeySize: return std::to_wstring(m.KeySize).c_str();
		case ColumnType::ValueSize: return std::to_wstring(m.ValueSize).c_str();
		case ColumnType::PinnedPathCount: return std::to_wstring(m.PinnedPathCount).c_str();
		case ColumnType::Flags: return std::format("0x{:X}", m.Flags).c_str();
	}
	return L"";
}

CString CMapsView::GetColumnTextMapData(int row, int column) const {
	auto index = m_MapList.GetSelectedIndex();
	if (index < 0)
		return L"";

	auto& m = m_MapData[row];
	auto& map = m_Maps[index];

	switch (static_cast<ColumnType>(GetColumnManager(m_MapDataList)->GetColumnTag(column))) {
		case ColumnType::Id: return std::to_wstring(m.Index + 1).c_str();
		case ColumnType::Key: return map.KeySize <= 8 ? StringHelper::FormatNumber(m.Key.get(), map.KeySize).c_str() : L"";
		case ColumnType::Value: return map.ValueSize <= 8 ? StringHelper::FormatNumber(m.Value.get(), map.ValueSize).c_str() : L"";
		case ColumnType::KeyHex: return StringHelper::BufferToHexString(m.Key.get(), map.KeySize).c_str();
		case ColumnType::ValueHex: return StringHelper::BufferToHexString(m.Value.get(), map.ValueSize).c_str();
		case ColumnType::ValueChars: return StringHelper::BufferToCharString(m.Value.get(), map.ValueSize).c_str();
	}
	return L"";
}

int CMapsView::GetRowImage(HWND hWnd, int row, int column) const {
	if (hWnd == m_MapDataList)
		return -1;
	return m_Maps[row].IsPerCpu() ? 0 : 1;
}

void CMapsView::OnStateChanged(HWND hWnd, int from, int to, UINT oldState, UINT newState) {
	if (hWnd == m_MapList) {
		if (newState & LVIS_SELECTED) {
			UpdateMapData(to);
		}
	}
}

void CMapsView::DoSortData(SortInfo const* si) {
	if (m_MapData.empty())
		return;

	auto sort = [&](auto const& p1, auto const& p2) {
		switch (static_cast<ColumnType>(GetColumnManager(m_MapDataList)->GetColumnTag(si->SortColumn))) {
			case ColumnType::Id: return SortHelper::Sort(p1.Index, p2.Index, si->SortAscending);
			case ColumnType::Key:
			case ColumnType::KeyHex:
				return SortHelper::Sort(p1.Key.get(), p2.Key.get(), m_CurrentMap.KeySize, si->SortAscending);

			case ColumnType::Value:
			case ColumnType::ValueHex:
			case ColumnType::ValueChars:
				return SortHelper::Sort(p1.Value.get(), p2.Value.get(), m_CurrentMap.KeySize, si->SortAscending);
		}
		return false;
		};

	std::ranges::sort(m_MapData, sort);
}

void CMapsView::UpdateMapData(int row) {
	if (row < 0) {
		m_MapDataList.SetItemCount(0);
		return;
	}

	auto& map = m_Maps[row];
	m_CurrentMap = map;

	m_MapData = BpfSystem::GetMapData(map.Id);
	m_MapDataList.SetItemCount((int)m_MapData.size());
}

void CMapsView::DoSort(SortInfo const* si) {
	if (si->hWnd == m_MapDataList) {
		DoSortData(si);
		return;
	}
		
	auto sort = [&](auto const& p1, auto const& p2) {
		switch (static_cast<ColumnType>(GetColumnManager(m_MapList)->GetColumnTag(si->SortColumn))) {
			case ColumnType::Name: return SortHelper::Sort(p1.Name, p2.Name, si->SortAscending);
			case ColumnType::Id: return SortHelper::Sort(p1.Id, p2.Id, si->SortAscending);
			case ColumnType::KeySize: return SortHelper::Sort(p1.KeySize, p2.KeySize, si->SortAscending);
			case ColumnType::ValueSize: return SortHelper::Sort(p1.ValueSize, p2.ValueSize, si->SortAscending);
			case ColumnType::MaxEntries: return SortHelper::Sort(p1.MaxEntries, p2.MaxEntries, si->SortAscending);
			case ColumnType::PinnedPathCount: return SortHelper::Sort(p1.PinnedPathCount, p2.PinnedPathCount, si->SortAscending);
			case ColumnType::Type: return SortHelper::Sort(StringHelper::MapTypeToString(p1.Type), StringHelper::MapTypeToString(p2.Type), si->SortAscending);
			case ColumnType::Flags: return SortHelper::Sort(p1.Flags, p2.Flags, si->SortAscending);
		}
		return false;
		};

	std::ranges::sort(m_Maps, sort);
}

int CMapsView::GetSaveColumnRange(HWND, int& start) const {
	start = 0;
	return 1;
}

LRESULT CMapsView::OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
	m_hWndClient = m_Splitter.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN);
	m_MapList.Create(m_Splitter, rcDefault, nullptr,
		WS_CHILD | WS_VISIBLE | LVS_OWNERDATA | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL);
	m_MapList.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_HEADERDRAGDROP);

	m_MapDataList.Create(m_Splitter, rcDefault, nullptr,
		WS_CHILD | WS_VISIBLE | LVS_OWNERDATA | LVS_REPORT | LVS_SHOWSELALWAYS);
	m_MapDataList.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_HEADERDRAGDROP);

	CImageList images;
	images.Create(16, 16, ILC_COLOR32 | ILC_MASK, 2, 2);
	UINT ids[] = { IDI_CPU, IDI_SYSTEM };
	for (auto id : ids)
		images.AddIcon(AtlLoadIconImage(id, 0, 16, 16));
	m_MapList.SetImageList(images, LVSIL_SMALL);

	m_Splitter.SetSplitterPosPct(25);
	m_Splitter.SetSplitterPanes(m_MapList, m_MapDataList);

	auto cm = GetColumnManager(m_MapList);
	cm->AddColumn(L"Name", LVCFMT_LEFT, 140, ColumnType::Name);
	cm->AddColumn(L"ID", LVCFMT_RIGHT, 60, ColumnType::Id);
	cm->AddColumn(L"Type", LVCFMT_LEFT, 100, ColumnType::Type);
	cm->AddColumn(L"Max Entries", LVCFMT_RIGHT, 80, ColumnType::MaxEntries);
	cm->AddColumn(L"Key Size", LVCFMT_RIGHT, 60, ColumnType::KeySize);
	cm->AddColumn(L"Value Size", LVCFMT_RIGHT, 60, ColumnType::ValueSize);
	cm->AddColumn(L"Pinned Paths", LVCFMT_RIGHT, 80, ColumnType::PinnedPathCount);
	cm->AddColumn(L"Flags", LVCFMT_RIGHT, 70, ColumnType::Flags);

	cm = GetColumnManager(m_MapDataList);
	cm->AddColumn(L"Dummy", 0, 0, 0);
	cm->AddColumn(L"#", LVCFMT_RIGHT, 60, ColumnType::Id);
	cm->AddColumn(L"Key", LVCFMT_RIGHT, 130, ColumnType::Key);
	cm->AddColumn(L"Key (Hex)", LVCFMT_LEFT, 200, ColumnType::KeyHex);
	cm->AddColumn(L"Value", LVCFMT_RIGHT, 130, ColumnType::Value);
	cm->AddColumn(L"Value (Hex)", LVCFMT_LEFT, 400, ColumnType::ValueHex);
	cm->AddColumn(L"Value (Chars)", LVCFMT_LEFT, 350, ColumnType::ValueChars);

	cm->DeleteColumn(0);

	m_MapDataList.SetFont(Frame()->GetMonoFont());

	m_MapDataList.GetHeader().SetFont(m_MapList.GetFont());
	Refresh();

	return 0;
}

LRESULT CMapsView::OnRefresh(WORD, WORD, HWND, BOOL&) {
	Refresh();
	return 0;
}

LRESULT CMapsView::OnUnpin(WORD, WORD, HWND, BOOL&) {
	auto n = m_MapList.GetSelectedIndex();
	if (n < 0)
		return 0;

	auto& map = m_Maps[n];
	if (map.PinnedPathCount == 0) {
		AtlMessageBox(m_hWnd, L"Map has no pinned paths", IDR_MAINFRAME, MB_ICONEXCLAMATION);
		return 0;
	}
	if (!BpfSystem::Unpin(map.Name.c_str())) {
		AtlMessageBox(m_hWnd, L"Failed to unpin map", IDR_MAINFRAME, MB_ICONERROR);
	}
	else {
		Refresh();
	}
	return 0;
}

LRESULT CMapsView::OnPinWithPath(WORD, WORD, HWND, BOOL&) {
	auto n = m_MapList.GetSelectedIndex();
	if (n < 0)
		return 0;

	CPinPathDlg dlg(CString(m_Maps[n].Name.c_str()));
	if (dlg.DoModal() == IDOK) {
		if (!BpfSystem::PinMap(m_Maps[n].Id, CStringA(dlg.GetPath())))
			AtlMessageBox(m_hWnd, L"Failed to pin map", IDR_MAINFRAME, MB_ICONERROR);
		else
			Refresh();
	}
	return 0;
}

LRESULT CMapsView::OnUnpinWithPath(WORD, WORD, HWND, BOOL&) {
	auto n = m_MapList.GetSelectedIndex();
	if (n < 0)
		return 0;

	CPinPathDlg dlg(CString(m_Maps[n].Name.c_str()));
	if (dlg.DoModal() == IDOK) {
		if (!BpfSystem::Unpin(CStringA(dlg.GetPath())))
			AtlMessageBox(m_hWnd, L"Failed to unpin map", IDR_MAINFRAME, MB_ICONERROR);
		else
			Refresh();
	}
	return 0;
}

LRESULT CMapsView::OnPin(WORD, WORD, HWND, BOOL&) {
	auto n = m_MapList.GetSelectedIndex();
	if (n < 0)
		return 0;

	if (!BpfSystem::PinMap(m_Maps[n].Id, m_Maps[n].Name.c_str()))
		AtlMessageBox(m_hWnd, L"Failed to pin map", IDR_MAINFRAME, MB_ICONERROR);
	else
		Refresh();
	return 0;
}

void CMapsView::Refresh() {
	m_Maps = BpfSystem::EnumMaps();

	m_MapList.SetItemCount((int)m_Maps.size());
	UpdateMapData(m_MapList.GetSelectedIndex());
}

LRESULT CMapsView::OnUpdateUI(UINT, WPARAM, LPARAM, BOOL&) {
	auto selected = m_MapList.GetSelectedIndex();
	auto& ui = Frame()->UI();
	ui.UIEnable(ID_BPF_PIN, selected >= 0);
	ui.UIEnable(ID_BPF_UNPIN, selected >= 0);
	ui.UIEnable(ID_EBPF_PINWITHPATH, selected >= 0);
	ui.UIEnable(ID_EBPF_UNPINWITHPATH, selected >= 0);

	return 0;
}
