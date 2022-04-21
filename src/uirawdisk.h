﻿/// @file uirawdisk.h
///
/// @brief ディスクID一覧
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef _UIRAWDISK_H_
#define _UIRAWDISK_H_

#include "common.h"
#include <wx/string.h>
#include <wx/splitter.h>
#include <wx/listctrl.h>
#include "diskd88.h"


#define USE_LIST_CTRL_ON_SECTOR_LIST

#ifndef USE_LIST_CTRL_ON_SECTOR_LIST
#include <wx/dataview.h>
#include <wx/clntdata.h>
#endif

class wxFileDataObject;
class L3DiskFrame;
class L3DiskRawTrack;
class L3DiskRawSector;

/// 分割ウィンドウ
class L3DiskRawPanel : public wxSplitterWindow
{
private:
	wxWindow *parent;
	L3DiskFrame *frame;

	L3DiskRawTrack *lpanel;
	L3DiskRawSector *rpanel;

	bool invert_data;	///< インポート・エクスポート時にデータを反転するか
	bool reverse_side;	///< インポート・エクスポート時にサイド番号を降順で行うか

public:
	L3DiskRawPanel(L3DiskFrame *parentframe, wxWindow *parentwindow);

	L3DiskRawTrack *GetLPanel() { return lpanel; }
	L3DiskRawSector *GetRPanel() { return rpanel; }

	/// トラックリストにデータを設定する
	void SetTrackListData(DiskD88Disk *disk, int side_num);
	/// トラックリストをクリアする
	void ClearTrackListData();
	/// トラックリストを再描画する
	void RefreshTrackListData();
	/// トラックリストが存在するか
	bool TrackListExists() const;
	/// トラックリストの選択行を返す
	int GetTrackListSelectedRow() const;

	/// セクタリストにデータを設定する
	void SetSectorListData(DiskD88Track *track);
	/// セクタリストをクリアする
	void ClearSectorListData();
	/// セクタリストの選択行を返す
	int GetSectorListSelectedRow() const;

	/// トラックリストとセクタリストを更新
	void RefreshAllData();

	/// クリップボードからペースト
	bool PasteFromClipboard();
	/// トラックからエクスポートダイアログ表示
	bool ShowExportTrackDialog();
	/// トラックへインポートダイアログ表示
	bool ShowImportTrackDialog();
	/// トラックへインポートダイアログ（トラックの範囲指定）表示
	bool ShowImportTrackRangeDialog(const wxString &path, int st_trk = -1, int st_sid = 0, int st_sec = 1);

	/// セクタからエクスポートダイアログ表示
	bool ShowExportDataFileDialog();
	/// セクタへインポートダイアログ表示
	bool ShowImportDataFileDialog();
	/// トラックのID一括変更
	void ModifyIDonTrack(int type_num);
	/// トラックの密度一括変更
	void ModifyDensityOnTrack();
	/// トラックのセクタ数一括変更
	void ModifySectorsOnTrack();
	/// トラックのセクタサイズ一括変更
	void ModifySectorSizeOnTrack();

	/// トラック or セクタのプロパティダイアログ表示
	bool ShowRawDiskAttr();
	/// セクタのプロパティダイアログ表示
	bool ShowSectorAttr();

	/// ファイル名
	wxString MakeFileName(DiskD88Sector *sector);
	/// ファイル名
	wxString MakeFileName(int st_c, int st_h, int st_r, int ed_c, int ed_h, int ed_r);

	/// フォントをセット
	void SetListFont(const wxFont &font);

	/// インポート・エクスポート時にデータを反転するか
	bool InvertData() const { return invert_data; }
	/// インポート・エクスポート時にデータを反転するか
	void InvertData(bool val) { invert_data = val; }
	/// インポート・エクスポート時にサイド番号を降順で行うか
	bool ReverseSide() const { return reverse_side; }
	/// インポート・エクスポート時にサイド番号を降順で行うか
	void ReverseSide(bool val) { reverse_side = val; }

	wxDECLARE_EVENT_TABLE();
	wxDECLARE_NO_COPY_CLASS(L3DiskRawPanel);
};

/// 左パネルのトラックリスト
class L3DiskRawTrack : public wxListView
{
private:
	L3DiskRawPanel *parent;
	L3DiskFrame *frame;

	DiskD88Disk *disk;
	int side_number;

	wxMenu *menuPopup;

	/// ファイルリスト作成（DnD, クリップボード用）
	bool CreateFileObject(wxString &tmp_dir_name, wxFileDataObject &file_object);
	/// ファイルリストを解放（DnD, クリップボード用）
	void ReleaseFileObject(const wxString &tmp_dir_name);

public:
	L3DiskRawTrack(L3DiskFrame *parentframe, L3DiskRawPanel *parentwindow);
	~L3DiskRawTrack();

	/// @name event procedures
	//@{
	/// トラックリストを選択
	void OnListItemSelected(wxListEvent& event);
	/// トラックリストをダブルクリック
	void OnListActivated(wxListEvent &event);
	/// トラックリスト右クリック
	void OnContextMenu(wxContextMenuEvent& event);
	/// トラックをエクスポート選択
	void OnExportTrack(wxCommandEvent& event);
	/// トラックにインポート選択
	void OnImportTrack(wxCommandEvent& event);
	/// データを反転するチェック選択
	void OnChangeInvertData(wxCommandEvent& event);
	/// サイドを逆転するチェック選択
	void OnChangeReverseSide(wxCommandEvent& event);
	/// トラックリストからドラッグ開始
	void OnBeginDrag(wxListEvent& event);
	/// ディスク上のID一括変更選択
	void OnModifyIDonDisk(wxCommandEvent& event);
	/// トラックのID一括変更選択
	void OnModifyIDonTrack(wxCommandEvent& event);
	/// ディスク上の密度一括変更選択
	void OnModifyDensityOnDisk(wxCommandEvent& event);
	/// トラックの密度一括変更選択
	void OnModifyDensityOnTrack(wxCommandEvent& event);
	/// トラックのセクタ数を一括変更選択
	void OnModifySectorsOnTrack(wxCommandEvent& event);
	/// トラックのセクタサイズを一括変更
	void OnModifySectorSizeOnTrack(wxCommandEvent& event);
	/// 現在のトラック以下を削除選択
	void OnDeleteTracksBelow(wxCommandEvent& event);
	/// トラックプロパティ選択
	void OnPropertyTrack(wxCommandEvent& event);
	/// キー押下
	void OnChar(wxKeyEvent& event);
	//@}

	/// 選択
	void SelectData(int row);
	/// トラックリストをセット
	void SetTracks(DiskD88Disk *newdisk, int newsidenum);
	/// トラックリストをセット
	void SetTracks();
	/// トラックリストを再セット
	void RefreshTracks();
	/// トラックリストをクリア
	void ClearTracks();

	/// トラックリスト上のポップアップメニュー表示
	void ShowPopupMenu();

	/// ファイルリストをドラッグ
	bool DragDataSourceForExternal();
	/// クリップボードへコピー
	bool CopyToClipboard();
	/// クリップボードからペースト
	bool PasteFromClipboard();

	/// トラックをエクスポート ダイアログ表示
	bool ShowExportTrackDialog();
	/// 指定したファイルにトラックデータをエクスポート
	bool ExportTrackDataFile(const wxString &path, int st_trk, int st_sid, int st_sec, int ed_trk, int ed_sid, int ed_sec, bool inv_data, bool rev_side);
	/// トラックにインポート ダイアログ表示
	bool ShowImportTrackDialog();
	/// 指定したファイルのファイル名にある数値から指定したトラックにインポートする
	bool ShowImportTrackRangeDialog(const wxString &path, int st_trk = -1, int st_sid = 0, int st_sec = 1);
	/// 指定したファイルから指定した範囲にトラックデータをインポート
	bool ImportTrackDataFile(const wxString &path, int st_trk, int st_sid, int st_sec, int ed_trk, int ed_sid, int ed_sec, bool inv_data, bool rev_side);
	/// ディスク全体のIDを変更
	void ModifyIDonDisk(int type_num);
	/// ディスク上の密度を一括変更
	void ModifyDensityOnDisk();
	/// トラック情報を表示
	void ShowTrackAttr();

	/// トラックを削除
	void DeleteTracks();

	/// ディスクを返す
	DiskD88Disk *GetDisk() const { return disk; }
	/// 選択行のトラックを返す
	DiskD88Track *GetSelectedTrack();
	/// 指定行のトラックを返す
	DiskD88Track *GetTrack(long row);
	/// 最初のトラックを返す
	DiskD88Track *GetFirstTrack();
	/// トラックのセクタ１を得る
	bool GetFirstSectorOnTrack(DiskD88Track **track, DiskD88Sector **sector);
	/// トラックの開始セクタ番号と終了セクタ番号を得る
	bool GetFirstAndLastSectorNumOnTrack(const DiskD88Track *track, int &start_sector, int &end_sector);

	enum {
		IDM_MENU_CHECK = 1,
		IDM_INVERT_DATA,
		IDM_REVERSE_SIDE,
		IDM_EXPORT_TRACK,
		IDM_IMPORT_TRACK,
		IDM_MODIFY_ID_C_DISK,
		IDM_MODIFY_ID_H_DISK,
		IDM_MODIFY_ID_R_DISK,
		IDM_MODIFY_ID_N_DISK,
		IDM_MODIFY_DENSITY_DISK,
		IDM_MODIFY_ID_C_TRACK,
		IDM_MODIFY_ID_H_TRACK,
		IDM_MODIFY_ID_R_TRACK,
		IDM_MODIFY_ID_N_TRACK,
		IDM_MODIFY_DENSITY_TRACK,
		IDM_MODIFY_SECTORS_TRACK,
		IDM_MODIFY_SIZE_TRACK,
		IDM_DELETE_TRACKS_BELOW,
		IDM_PROPERTY_TRACK,
	};

	wxDECLARE_EVENT_TABLE();
	wxDECLARE_NO_COPY_CLASS(L3DiskRawTrack);
};


/// セクタリストの各アイテム
class L3DiskRawSectorItem
{
public:
	wxString filename;
	wxString attribute;
	int size;
	int groups;
public:
	L3DiskRawSectorItem(const wxString &newname, const wxString &newattr, int newsize, int newgrps);
	~L3DiskRawSectorItem() {}
};

WX_DECLARE_OBJARRAY(L3DiskRawSectorItem, L3DiskRawSectorItems);

#ifndef USE_LIST_CTRL_ON_SECTOR_LIST
/// セクタリストの挙動を設定
class L3DiskRawListStoreDerivedModel : public wxDataViewListStore
{
public:
    virtual bool IsEnabledByRow(unsigned int row, unsigned int col) const;
};

#define L3SectorListColumn	wxDataViewColumn*
#define L3SectorListItem	wxDataViewItem
#define L3SectorListItems	wxDataViewItemArray
#define L3SectorListEvent	wxDataViewEvent
#else
#define L3SectorListColumn	long
#define L3SectorListItem	long
#define L3SectorListItems	wxArrayLong
#define L3SectorListEvent	wxListEvent
#endif

/// 右パネルのセクタリスト
#ifndef USE_LIST_CTRL_ON_SECTOR_LIST
class L3DiskRawSector : public wxDataViewListCtrl
#else
class L3DiskRawSector : public wxListCtrl
#endif
{
private:
	L3DiskRawPanel *parent;
	L3DiskFrame *frame;

	wxMenu *menuPopup;

	DiskD88Track *track;

//	L3SectorListItem selected_item;

	bool initialized;

	/// ファイルリスト作成（DnD, クリップボード用）
	bool CreateFileObject(wxString &tmp_dir_name, wxFileDataObject &file_object);
	/// ファイルリストを解放（DnD, クリップボード用）
	void ReleaseFileObject(const wxString &tmp_dir_name);

public:
	L3DiskRawSector(L3DiskFrame *parentframe, L3DiskRawPanel *parent);
	~L3DiskRawSector();

	/// @name event procedures
	//@{
	/// セクタリスト右クリック
	void OnItemContextMenu(L3SectorListEvent& event);
	/// 右クリック
	void OnContextMenu(wxContextMenuEvent& event);
	/// セクタリスト ダブルクリック
	void OnItemActivated(L3SectorListEvent& event);
	/// セクタリスト選択
	void OnSelectionChanged(L3SectorListEvent& event);
	/// セクタリストからドラッグ開始
	void OnBeginDrag(L3SectorListEvent& event);
	/// セクタリスト エクスポート選択
	void OnExportFile(wxCommandEvent& event);
	/// セクタリスト インポート選択
	void OnImportFile(wxCommandEvent& event);
	/// データを反転するチェック選択
	void OnChangeInvertData(wxCommandEvent& event);
	/// トラック上のID一括変更選択
	void OnModifyIDonTrack(wxCommandEvent& event);
	/// トラック上の密度一括変更選択
	void OnModifyDensityOnTrack(wxCommandEvent& event);
	/// トラック上のセクタ数一括変更選択
	void OnModifySectorsOnTrack(wxCommandEvent& event);
	/// トラック上のセクタサイズ一括変更選択
	void OnModifySectorSizeOnTrack(wxCommandEvent& event);
	/// セクタ追加選択
	void OnAppendSector(wxCommandEvent& event);
	/// セクタ削除選択
	void OnDeleteSector(wxCommandEvent& event);
	/// トラック上のセクタ一括削除選択
	void OnDeleteSectorsOnTrack(wxCommandEvent& event);
	/// セクタプロパティ選択
	void OnPropertySector(wxCommandEvent& event);
	/// セクタリスト上でキー押下
	void OnChar(wxKeyEvent& event);
	//@}

	/// ポップアップメニュー表示
	void ShowPopupMenu();

	// セクタリスト選択
	void SelectItem(DiskD88Sector *sector);
	// セクタリスト非選択
	void UnselectItem();

	/// セクタリストにデータをセット
	void SetSectors(DiskD88Track *newtrack);
	/// セクタリストをリフレッシュ
	void RefreshSectors();
	/// セクタリストをクリア
	void ClearSectors();
	/// 選択しているセクタを返す
	DiskD88Sector *GetSelectedSector(int *pos = NULL);
	/// セクタを返す
	DiskD88Sector *GetSector(const L3SectorListItem &item);

	/// ファイルリストをドラッグ
	bool DragDataSourceForExternal();
	/// クリップボードへコピー
	bool CopyToClipboard();
	/// クリップボードからペースト
	bool PasteFromClipboard();

	/// エクスポートダイアログ表示
	bool ShowExportDataFileDialog();
	/// 指定したファイルにセクタのデータをエクスポート
	bool ExportDataFile(const wxString &path, DiskD88Sector *sector);
	/// インポートダイアログ表示
	bool ShowImportDataFileDialog();
	/// セクタ情報プロパティダイアログ表示
	bool ShowSectorAttr();

	/// トラック上のIDを一括変更
	void ModifyIDonTrack(int type_num);
	/// トラック上の密度を一括変更
	void ModifyDensityOnTrack();
	/// トラック上のセクタ数を一括変更
	void ModifySectorsOnTrack();
	/// トラック上のセクタサイズを一括変更
	void ModifySectorSizeOnTrack();

	/// セクタを追加ダイアログを表示
	void ShowAppendSectorDialog();
	/// セクタを削除
	void DeleteSector();
	/// トラック上のセクタを一括削除
	void DeleteSectorsOnTrack();

	/// 選択行を返す
	int  GetListSelectedRow() const;
	/// 選択行数を返す
	int  GetListSelectedCount() const;
	/// 選択行を配列で返す
	int  GetListSelections(L3SectorListItems &arr) const;

	enum {
		IDM_MENU_CHECK = 1,
		IDM_INVERT_DATA,
		IDM_REVERSE_SIDE,
		IDM_EXPORT_FILE,
		IDM_IMPORT_FILE,
		IDM_MODIFY_ID_C_TRACK,
		IDM_MODIFY_ID_H_TRACK,
		IDM_MODIFY_ID_R_TRACK,
		IDM_MODIFY_ID_N_TRACK,
		IDM_MODIFY_DENSITY_TRACK,
		IDM_MODIFY_SECTORS_TRACK,
		IDM_MODIFY_SIZE_TRACK,
		IDM_APPEND_SECTOR,
		IDM_DELETE_SECTOR,
		IDM_DELETE_SECTORS_BELOW,
		IDM_PROPERTY_SECTOR,
	};

	wxDECLARE_EVENT_TABLE();
};

#endif /* _UIRAWDISK_H_ */

