// View.cpp : implementation of the CView class
//
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "resource.h"
#include "ProgramsView.h"
#include "StringHelper.h"
#include <SortHelper.h>

BOOL CProgramsView::PreTranslateMessage(MSG* pMsg) {
	pMsg;
	return FALSE;
}

CString CProgramsView::GetColumnText(HWND hWnd, int row, int column) const {
	auto& p = m_Programs[row];
	switch (static_cast<ColumnType>(GetColumnManager(m_List)->GetColumnTag(column))) {
		case ColumnType::Name: return CString(p.Name.c_str());
		case ColumnType::Id: return std::to_wstring(p.Id).c_str();
		case ColumnType::Type: return StringHelper::ProgramTypeToString(p.Type);
		case ColumnType::MapCount: return std::to_wstring(p.MapCount).c_str();
		case ColumnType::LinkCount: return std::to_wstring(p.LinkCount).c_str();
		case ColumnType::PinnedPathCount: return std::to_wstring(p.PinnedPathCount).c_str();
		case ColumnType::FileName: return CString(p.FileName.c_str());
		case ColumnType::Section: return CString(p.Section.c_str());
		case ColumnType::MapIds: return StringHelper::VectorToString(p.MapIds);
		case ColumnType::ExeType: return StringHelper::ExeutionTypeToString(p.ExecutionType);
		case ColumnType::GuidType: return StringHelper::GuidToString(p.UuidType);
	}

	return CString();
}

int CProgramsView::GetRowImage(HWND, int row, int) const {
	return 0;
}

void CProgramsView::DoSort(SortInfo const* si) {
	auto sort = [&](auto const& p1, auto const& p2) {
		switch (static_cast<ColumnType>(GetColumnManager(m_List)->GetColumnTag(si->SortColumn))) {
			case ColumnType::Name: return SortHelper::Sort(p1.Name, p2.Name, si->SortAscending);
			case ColumnType::Id: return SortHelper::Sort(p1.Id, p2.Id, si->SortAscending);
			case ColumnType::FileName: return SortHelper::Sort(p1.FileName, p2.FileName, si->SortAscending);
			case ColumnType::Section: return SortHelper::Sort(p1.Section, p2.Section, si->SortAscending);
			case ColumnType::MapCount: return SortHelper::Sort(p1.MapCount, p2.MapCount, si->SortAscending);
			case ColumnType::LinkCount: return SortHelper::Sort(p1.LinkCount, p2.LinkCount, si->SortAscending);
			case ColumnType::Type: return SortHelper::Sort(StringHelper::ProgramTypeToString(p1.Type), StringHelper::ProgramTypeToString(p2.Type), si->SortAscending);
		}
		return false;
		};
	std::ranges::sort(m_Programs, sort);
}

LRESULT CProgramsView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	m_hWndClient = m_List.Create(m_hWnd, rcDefault, nullptr,
		WS_CHILD | WS_VISIBLE | LVS_OWNERDATA | LVS_REPORT);
	m_List.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_HEADERDRAGDROP);

	CImageList images;
	images.Create(16, 16, ILC_COLOR32 | ILC_MASK, 2, 2);
	UINT ids[] = { IDR_MAINFRAME };
	for (auto id : ids)
		images.AddIcon(AtlLoadIconImage(id, 0, 16, 16));
	m_List.SetImageList(images, LVSIL_SMALL);

	auto cm = GetColumnManager(m_List);
	cm->AddColumn(L"Name", LVCFMT_LEFT, 140, ColumnType::Name);
	cm->AddColumn(L"ID", LVCFMT_RIGHT, 60, ColumnType::Id);
	cm->AddColumn(L"Type", LVCFMT_LEFT, 130, ColumnType::Type);
	cm->AddColumn(L"Maps", LVCFMT_RIGHT, 60, ColumnType::MapCount);
	cm->AddColumn(L"Links", LVCFMT_RIGHT, 60, ColumnType::LinkCount);
	cm->AddColumn(L"Pinned Paths", LVCFMT_RIGHT, 80, ColumnType::PinnedPathCount);
	cm->AddColumn(L"Filename", LVCFMT_LEFT, 140, ColumnType::FileName);
	cm->AddColumn(L"Execution Type", LVCFMT_LEFT, 140, ColumnType::ExeType);
	cm->AddColumn(L"Map IDs", LVCFMT_LEFT, 120, ColumnType::MapIds);
	cm->AddColumn(L"Section", LVCFMT_LEFT, 140, ColumnType::Section);
	cm->AddColumn(L"Type GUID", LVCFMT_LEFT, 200, ColumnType::GuidType);

	Refresh();

	return 0;
}

LRESULT CProgramsView::OnRefresh(WORD, WORD, HWND, BOOL&) {
	Refresh();

	return 0;
}

void CProgramsView::Refresh() {
	m_Programs = BpfSystem::EnumPrograms();
	m_List.SetItemCount((int)m_Programs.size());
}
