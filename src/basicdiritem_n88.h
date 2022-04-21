﻿/// @file basicdiritem_n88.h
///
/// @brief disk basic directory item for N88-BASIC
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef _BASICDIRITEM_N88_H_
#define _BASICDIRITEM_N88_H_

#include "basicdiritem_fat8.h"


/// N88-BASIC

extern const char *gTypeNameN88_1[];
enum en_type_name_n88_1 {
	TYPE_NAME_N88_ASCII = 0,
	TYPE_NAME_N88_BINARY = 1,
	TYPE_NAME_N88_MACHINE = 2,
	TYPE_NAME_N88_RANDOM = 3,
};
enum en_file_type_n88 {
	FILETYPE_N88_ASCII = 0x00,
	FILETYPE_N88_BINARY = 0x80,
	FILETYPE_N88_MACHINE = 0x01
};

extern const char *gTypeNameN88_2[];
enum en_type_name_n88_2 {
	TYPE_NAME_N88_READ_ONLY = 0,
	TYPE_NAME_N88_READ_WRITE = 1,
	TYPE_NAME_N88_ENCRYPTED = 2
};
enum en_data_type_mask_n88 {
	DATATYPE_MASK_N88_READ_ONLY = 0x10,
	DATATYPE_MASK_N88_READ_WRITE = 0x40,
	DATATYPE_MASK_N88_ENCRYPTED = 0x20
};

/// ディレクトリ１アイテム N88-BASIC
class DiskBasicDirItemN88 : public DiskBasicDirItemFAT8
{
private:
	DiskBasicDirItemN88() : DiskBasicDirItemFAT8() {}
	DiskBasicDirItemN88(const DiskBasicDirItemN88 &src) : DiskBasicDirItemFAT8(src) {}

	/// ファイル名を格納する位置を返す
	wxUint8 *GetFileNamePos(size_t &size, size_t &len) const;
	/// 拡張子を格納する位置を返す
	wxUint8 *GetFileExtPos(size_t &len) const;
//	/// ファイル名を格納するバッファサイズを返す
//	int		GetFileNameSize(bool *invert = NULL) const;
//	/// 拡張子を格納するバッファサイズを返す
//	int		GetFileExtSize(bool *invert = NULL) const;
	/// 属性１を返す
	int		GetFileType1() const;
	/// 属性１のセット
	void	SetFileType1(int val);
	/// 使用しているアイテムか
	bool	CheckUsed(bool unuse);

	/// 属性からリストの位置を返す(プロパティダイアログ用)
	int	    GetFileType1Pos() const;
	/// 属性からリストの位置を返す(プロパティダイアログ用)
	int	    GetFileType2Pos() const;
	/// リストの位置から属性を返す(プロパティダイアログ用)
	int		CalcFileTypeFromPos(int pos);
	/// インポート時ダイアログ表示前にファイルの属性を設定
	void	SetFileTypeForAttrDialog(int show_flags, const wxString &name, int &file_type_1, int &file_type_2);

public:
	DiskBasicDirItemN88(DiskBasic *basic);
	DiskBasicDirItemN88(DiskBasic *basic, DiskD88Sector *sector, wxUint8 *data);
	DiskBasicDirItemN88(DiskBasic *basic, int num, int track, int side, DiskD88Sector *sector, int secpos, wxUint8 *data, bool &unuse);

	/// ディレクトリアイテムのチェック
	bool			Check(bool &last);

	/// ENDマークがあるか(一度も使用していないか)
	bool			HasEndMark();
	/// 次のアイテムにENDマークを入れる
	void			SetEndMark(DiskBasicDirItem *next_item);

	/// 属性を設定
	void			SetFileAttr(const DiskBasicFileType &file_type);

	/// 属性を返す
	DiskBasicFileType GetFileAttr() const;

	/// 属性の文字列を返す(ファイル一覧画面表示用)
	wxString		GetFileAttrStr() const;

	/// ファイルサイズをセット
	void			SetFileSize(int val);

	/// 最初のグループ番号をセット
	void			SetStartGroup(wxUint32 val);
	/// 最初のグループ番号を返す
	wxUint32		GetStartGroup() const;


	/// ディレクトリアイテムのサイズ
	size_t			GetDataSize() const;

	/// ファイルの終端コードをチェックする必要があるか
	bool			NeedCheckEofCode();
//	/// データをエクスポートする前に必要な処理
//	bool			PreExportDataFile(wxString &filename);
	/// セーブ時にファイルサイズを再計算する ファイルの終端コードが必要な場合など
	int				RecalcFileSizeOnSave(wxInputStream *istream, int file_size);

	/// ディレクトリをクリア ファイル新規作成時
	void			ClearData();


	/// @name プロパティダイアログ用
	//@{
	/// ダイアログ内の属性部分のレイアウトを作成
	void	CreateControlsForAttrDialog(IntNameBox *parent, int show_flags, const wxString &file_path, wxBoxSizer *sizer, wxSizerFlags &flags);
	/// 属性を変更した際に呼ばれるコールバック
	void	ChangeTypeInAttrDialog(IntNameBox *parent);
	/// 機種依存の属性を設定する
	bool	SetAttrInAttrDialog(const IntNameBox *parent, DiskBasicError &errinfo);
	//@}
};

#endif /* _BASICDIRITEM_N88_H_ */
