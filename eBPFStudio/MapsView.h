#pragma once

#include <FrameView.h>
#include <VirtualListView.h>
#include "IMainFrame.h"
#include <eBPF.h>
#include <CustomSplitterWindow.h>

class CMapsView :
	public CFrameView<CMapsView, IMainFrame>,
	public CVirtualListView<CMapsView> {
public:
	using CFrameView::CFrameView;

	DECLARE_WND_CLASS(nullptr)

	CString GetColumnText(HWND hWnd, int row, int column) const;
	CString GetColumnTextMapData(int row, int column) const;
	int GetRowImage(HWND hWnd, int row, int column) const;
	void OnStateChanged(HWND hWnd, int from, int to, UINT oldState, UINT newState);
	void DoSort(SortInfo const* si);

	BEGIN_MSG_MAP(CMapsView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		CHAIN_MSG_MAP(CVirtualListView<CMapsView>)
		CHAIN_MSG_MAP(BaseFrame)
	ALT_MSG_MAP(1)
		COMMAND_ID_HANDLER(ID_VIEW_REFRESH, OnRefresh)
	END_MSG_MAP()

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

protected:
	void UpdateMapData(int row);

	enum class ColumnType {
		Name, Id, KeySize, ValueSize, Type, PinnedPathCount, MaxEntries, Flags,
		Key, KeyHex, Value, ValueHex,
	};

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void Refresh();

private:
	CCustomHorSplitterWindow m_Splitter;
	CListViewCtrl m_MapList;
	CListViewCtrl m_MapDataList;
	std::vector<BpfMap> m_Maps;
	std::vector<BpfMapItem> m_MapData;
};
