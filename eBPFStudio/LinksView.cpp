#include "pch.h"
#include "LinksView.h"
#include "StringHelper.h"

CString CLinksView::GetColumnText(HWND hWnd, int row, int column) const {
	auto& link = m_Links[row];
	switch (static_cast<ColumnType>(GetColumnManager(m_List)->GetColumnTag(column))) {
		case ColumnType::Id: return std::to_wstring(link.Id).c_str();
		case ColumnType::ProgramId: return std::to_wstring(link.ProgramId).c_str();
		case ColumnType::Type: return StringHelper::LinkTypeToString(link.Type);
		case ColumnType::AttachType: return StringHelper::AttachTypeToString(link.AttachType);
		case ColumnType::Data: return std::format("0x{:X}", link.AttachData).c_str();
		case ColumnType::ProgramGuid: return StringHelper::GuidToString(link.ProgramTypeUuid);
		case ColumnType::AttachTypeGuid: return StringHelper::GuidToString(link.AttachTypeUuid);
	}
	return L"";
}

LRESULT CLinksView::OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
	m_hWndClient = m_List.Create(m_hWnd, rcDefault, nullptr,
		WS_CHILD | WS_VISIBLE | LVS_OWNERDATA | LVS_REPORT);
	m_List.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_HEADERDRAGDROP);

	auto cm = GetColumnManager(m_List);
	cm->AddColumn(L"", 0, 0, 0);
	cm->AddColumn(L"ID", LVCFMT_RIGHT, 60, ColumnType::Id);
	cm->AddColumn(L"Type", LVCFMT_LEFT, 100, ColumnType::Type);
	cm->AddColumn(L"Attach Type", LVCFMT_LEFT, 150, ColumnType::AttachType);
	cm->AddColumn(L"Program ID", LVCFMT_RIGHT, 70, ColumnType::ProgramId);
	cm->AddColumn(L"Data", LVCFMT_RIGHT, 100, ColumnType::Data);
	cm->AddColumn(L"Attach Type GUID", LVCFMT_LEFT, 200, ColumnType::AttachTypeGuid);
	cm->AddColumn(L"Program Type GUID", LVCFMT_LEFT, 200, ColumnType::ProgramGuid);
	cm->DeleteColumn(0);

	Refresh();

	return 0;
}

LRESULT CLinksView::OnRefresh(WORD, WORD, HWND, BOOL&) {
    Refresh();

    return 0;
}

void CLinksView::Refresh() {
	m_Links = BpfSystem::EnumLinks();
	m_List.SetItemCount((int)m_Links.size());
}
