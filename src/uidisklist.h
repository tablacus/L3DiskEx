﻿/// @file uidisklist.h
///
/// @brief ディスクリスト
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef _UIDISKLIST_H_
#define _UIDISKLIST_H_

#include "common.h"
#include <wx/string.h>

#define USE_TREE_CTRL_ON_DISK_LIST

#ifndef USE_TREE_CTRL_ON_DISK_LIST
#include <wx/dataview.h>
#include <wx/clntdata.h>
#else
#include <wx/treectrl.h>
#include <wx/treebase.h>
#endif


class L3DiskFrame;
class DiskD88File;
class DiskD88Disks;
class DiskD88Disk;
class DiskBasic;
class DiskBasicParam;
class DiskBasicDirItem;
class DiskBasicDirItems;

enum L3DiskPositionDataNum {
	CD_DISKNUM_ROOT = -1,
	CD_TYPENUM_NODE = -1,
	CD_TYPENUM_NODE_AB = -2,
};

//////////////////////////////////////////////////////////////////////

/// ディスク情報に紐づける属性
#ifndef USE_TREE_CTRL_ON_DISK_LIST
class L3DiskPositionData : public wxClientData
#else
class L3DiskPositionData : public wxTreeItemData
#endif
{
private:
	int disknum;
	int typenum;
	int sidenum;
	int pos;
	bool editable;
	bool shown;
	DiskBasicDirItem *ditem;
public:
	L3DiskPositionData(int n_disknum, int n_typenum, int n_sidenum, int n_pos, bool n_editable, DiskBasicDirItem *n_ditem = NULL);
	~L3DiskPositionData();

	int  GetDiskNumber() const { return disknum; }
	int  GetTypeNumber() const { return typenum; }
	void SetTypeNumber(int val) { typenum = val; }
	int  GetSideNumber() const { return sidenum; }
	int  GetPosition() const { return pos; }
	bool GetEditable() const { return editable; }
	/// このノードの子供を表示したか
	bool IsShown() const { return shown; }
	/// このノードの子供を設定して表示したらtrueにする
	void Shown(bool val) { shown = val; }
	DiskBasicDirItem *GetDiskBasicDirItem() { return ditem; }
	void SetDiskBasicDirItem(DiskBasicDirItem *val) { ditem = val; }
};

//////////////////////////////////////////////////////////////////////

#ifndef USE_TREE_CTRL_ON_DISK_LIST
/// ディスク１枚部分のツリーアイテムの挙動を設定する
class L3DiskTreeStoreModel : public wxDataViewTreeStore
{
private:
	L3DiskFrame *frame;
public:
	L3DiskTreeStoreModel(L3DiskFrame *parentframe);

	/// ディスク名編集できるか
	virtual bool IsEnabled(const wxDataViewItem &item, unsigned int col) const;
	/// ディスク名を変更した
	virtual bool SetValue(const wxVariant &variant, const wxDataViewItem &item, unsigned int col);
};

#define L3DiskListItem  wxDataViewItem
#define L3DiskListEvent wxDataViewEvent

#else

#define L3DiskListItem  wxTreeItemId
#define L3DiskListEvent wxTreeEvent

#endif

//////////////////////////////////////////////////////////////////////

/// ツリーコントロール
#ifndef USE_TREE_CTRL_ON_DISK_LIST
class L3DiskTreeCtrl: public wxDataViewTreeCtrl
#else
class L3DiskTreeCtrl: public wxTreeCtrl
#endif
{
public:
	L3DiskTreeCtrl(wxWindow *parentwindow, wxWindowID id);

	/// ツリーノードを選択
	void SelectTreeNode(const L3DiskListItem &node);
	/// ツリーノードが子供を持つか
	bool TreeNodeHasChildren(const L3DiskListItem &node);
	/// ツリーノードの子供の数を返す
	int  GetTreeChildCount(const L3DiskListItem &parent);
	/// ツリーノードを編集
	void EditTreeNode(const L3DiskListItem &node);
	/// ツリーノードを削除
	void DeleteTreeNode(const L3DiskListItem &node);
	/// 親ツリーノードを返す
	L3DiskListItem GetParentTreeNode(const L3DiskListItem &node);
	/// ルートノードを追加する
	L3DiskListItem AddRootTreeNode(const wxString &text, int def_icon = -1, int sel_icon = -1, L3DiskPositionData *n_data = NULL);
	/// ノードを追加する
	L3DiskListItem AddTreeContainer(const L3DiskListItem &parent, const wxString &text, int def_icon = -1, int sel_icon = -1, L3DiskPositionData *n_data = NULL);
	/// ノードを追加する
	L3DiskListItem AddTreeNode(const L3DiskListItem &parent, const wxString &text, int def_icon = -1, int sel_icon = -1, L3DiskPositionData *n_data = NULL);
};

//////////////////////////////////////////////////////////////////////

/// 左Panel ディスク情報
class L3DiskList: public L3DiskTreeCtrl
{
private:
	wxWindow *parent;
	L3DiskFrame *frame;

	wxMenu *menuPopup;

	DiskD88Disk *selected_disk;
	bool disk_selecting;

	bool initialized;

	L3DiskListItem root_node;

	/// 選択位置のディスクイメージ
	L3DiskListItem SetSelectedItemAtDiskImage();
	/// サブキャプション
	void SubCaption(int type, int side_number, wxString &caption) const;

public:
	L3DiskList(L3DiskFrame *parentframe, wxWindow *parent);
	~L3DiskList();

	enum SetDataOnItemNodeFlags {
		NODE_SELECTED = 0,
		NODE_EXPANDED
	};

	/// @name event procedures
	//@{
	/// 右クリック選択
	void OnContextMenu(L3DiskListEvent& event);
	/// ツリーアイテム選択
	void OnSelectionChanged(L3DiskListEvent& event);
	/// アイテム展開
	void OnItemExpanding(L3DiskListEvent& event);
	/// アイテム編集開始
	void OnStartEditing(L3DiskListEvent& event);
	/// アイテム編集終了
	void OnEditingDone(L3DiskListEvent& event);
	/// ディスクを保存選択
	void OnSaveDisk(wxCommandEvent& event);
	/// ディスクを新規に追加選択
	void OnAddNewDisk(wxCommandEvent& event);
	/// ディスクをファイルから追加選択
	void OnAddDiskFromFile(wxCommandEvent& event);
	/// ディスクイメージを置換選択
	void OnReplaceDisk(wxCommandEvent& event);
	/// ディスクを削除選択
	void OnDeleteDisk(wxCommandEvent& event);
	/// ディスク名を変更選択
	void OnRenameDisk(wxCommandEvent& event);
	/// 初期化選択
	void OnInitializeDisk(wxCommandEvent& event);
	/// フォーマット選択
	void OnFormatDisk(wxCommandEvent& event);
	/// プロパティ選択
	void OnPropertyDisk(wxCommandEvent& event);
	/// BASIC情報選択
	void OnPropertyBasic(wxCommandEvent& event);
	/// キー入力
	void OnChar(wxKeyEvent& event);
	//@}

	/// @name functions
	//@{
	/// ポップアップメニュー表示
	void ShowPopupMenu();

	/// 再選択
	void ReSelect(const DiskBasicParam *newparam = NULL);
	/// ツリーを選択
	void ChangeSelection(L3DiskListItem &node, const DiskBasicParam *newparam = NULL);
	/// ディスクを指定して選択状態にする
	void ChangeSelection(int disk_number, int side_number);
	/// ツリーを展開
	void ExpandItemNode(L3DiskListItem &node);
	/// 指定ノードにデータを設定する
	void SetDataOnItemNode(const L3DiskListItem &node, SetDataOnItemNodeFlags flag, const DiskBasicParam *newparam = NULL);
	/// DISK BASICをアタッチ＆解析
	bool ParseDiskBasic(const L3DiskListItem &node, L3DiskPositionData *cd, DiskD88Disk *newdisk, int newsidenum, const DiskBasicParam *newparam = NULL);
	/// 選択しているディスクの子供を削除
	void DeleteChildrenOnSelectedDisk();
	/// 選択しているディスクのルートを初期化＆再選択
	void RefreshSelectedDisk(const DiskBasicParam *newparam = NULL);
	/// 選択しているサイドを再選択
	void RefreshSelectedSide(const DiskBasicParam *newparam = NULL);
	/// ファイル名をリストにセット
	void SetFileName();
	/// ファイル名をリストにセット
	void SetFileName(const wxString &filename);
//	/// ファイル名をリストにセット
//	void SetFileName(const wxString &filename, L3DiskNameStrings &disknames);
	/// リストをクリア
	void ClearFileName();
	/// ファイルパスをリストにセット
	void SetFilePath(const wxString &filename);

	/// ディスク番号と一致するノードをさがす
	L3DiskListItem FindNodeByDiskNumber(const L3DiskListItem &node, int disk_number, int depth = 0);
	/// ディスク番号＆サイド番号と一致するノードをさがす
	L3DiskListItem FindNodeByDiskAndSideNumber(const L3DiskListItem &node, int disk_number, int side_number, int depth = 0);

	/// ディスクの初期化
	bool InitializeDisk();
	/// ディスクを論理フォーマット
	bool FormatDisk();

	/// ディスクをファイルに保存ダイアログ
	void ShowSaveDiskDialog();
	/// ディスクを置換
	void ReplaceDisk();
	/// ディスクをファイルから削除
	bool DeleteDisk();
	/// ディスク名を変更
	void RenameDisk();
	/// ディスク情報ダイアログ
	void ShowDiskAttr();
	/// 選択位置のディスク名をセット
	void SetDiskName(const wxString &val);
	/// キャラクターコード変更
	void ChangeCharCode(int sel);
	/// フォントをセット
	void SetListFont(const wxFont &font);
	//@}

	/// @name properties
	//@{
	/// 選択しているディスクイメージのディスク番号を返す
	int  GetSelectedDiskNumber();
	/// 選択しているディスクイメージのサイド番号を返す
	int  GetSelectedDiskSide();
	/// 選択しているディスクイメージのディスク番号とサイド番号を返す
	void GetSelectedDisk(int &disk_number, int &side_number);
	/// ディスクイメージを選択しているか
	bool IsSelectedDiskImage();
	/// ディスクを選択しているか
	bool IsSelectedDisk();
	/// ディスクを選択しているか(AB面どちらか)
	bool IsSelectedDiskSide();
	//@}

	enum {
		IDM_SAVE_DISK = 1,
		IDM_ADD_DISK_NEW,
		IDM_ADD_DISK_FROM_FILE,
		IDM_REPLACE_DISK_FROM_FILE,
		IDM_DELETE_DISK_FROM_FILE,
		IDM_RENAME_DISK,
		IDM_INITIALIZE_DISK,
		IDM_FORMAT_DISK,
		IDM_PROPERTY_DISK,
		IDM_PROPERTY_BASIC,
	};

	wxDECLARE_EVENT_TABLE();
};

#endif /* _UIDISKLIST_H_ */

