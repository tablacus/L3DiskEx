﻿/// @file basictype_sdos.h
///
/// @brief disk basic type for S-DOS
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef _BASICTYPE_SDOS_H_
#define _BASICTYPE_SDOS_H_

#include "common.h"
#include "basiccommon.h"
#include "basictype.h"


/** @class DiskBasicTypeSDOS

@brief S-DOSの処理

DiskBasicParam
@li IPLCompareString : OS判別用検索文字列 セクタ１を検索
@li DefaultStartAddress : BASIC指定時の開始アドレス
@li DefaultExecuteAddress : BASIC指定時の実行アドレス

*/
class DiskBasicTypeSDOS : public DiskBasicType
{
private:
	DiskBasicTypeSDOS() : DiskBasicType() {}
	DiskBasicTypeSDOS(const DiskBasicType &src) : DiskBasicType(src) {}

	wxUint32 m_empty_group_num;	///< 空き開始グループ

public:
	DiskBasicTypeSDOS(DiskBasic *basic, DiskBasicFat *fat, DiskBasicDir *dir);

	/// @name access to FAT area
	//@{
	/// @brief FAT位置をセット
	void		SetGroupNumber(wxUint32 num, wxUint32 val);
	/// @brief FAT位置を返す
	wxUint32	GetGroupNumber(wxUint32 num) const;
	/// @brief 空きFAT位置を返す
	wxUint32	GetEmptyGroupNumber();
	/// @brief 次の空きFAT位置を返す
	wxUint32	GetNextEmptyGroupNumber(wxUint32 curr_group);
	//@}

	/// @name check / assign FAT area
	//@{
	/// @brief FATエリアをチェック
	double 	CheckFat(bool is_formatting);
	/// @brief ディスクから各パラメータを取得＆必要なパラメータを計算
	double	ParseParamOnDisk(DiskD88Disk *disk, bool is_formatting);
	//@}

	/// @name check / assign directory area
	//@{
	/// @brief ディレクトリエリアのサイズに達したらアサイン終了するか
	/// @retval  0 : 終了しない
	/// @retval  1 : 強制的に未使用とする アサインは継続
	/// @retval -1 : 強制的にアサイン終了する
	int		FinishAssigningDirectory(int size) const;
	//@}

	/// @name disk size
	//@{
	/// @brief 使用可能なディスクサイズを得る
	void	GetUsableDiskSize(int &disk_size, int &group_size) const;
	/// @brief 残りディスクサイズを計算
	void	CalcDiskFreeSize(bool wrote);
	//@}

	/// @name file chain
	//@{
	/// @brief データサイズ分のグループを確保する
	int		AllocateUnitGroups(int fileunit_num, DiskBasicDirItem *item, int data_size, AllocateGroupFlags flags, DiskBasicGroups &group_items);
	/// @brief グループ番号から開始セクタ番号を得る
	int		GetStartSectorFromGroup(wxUint32 group_num);
	//@}

	/// @name directory
	//@{
	//@}

	/// @name format
	//@{
	/// @brief フォーマットできるか
	bool	SupportFormatting() const { return false; }
	/// @brief セクタデータを指定コードで埋める
	void	FillSector(DiskD88Track *track, DiskD88Sector *sector);
	/// @brief セクタデータを埋めた後の個別処理
	bool	AdditionalProcessOnFormatted(const DiskBasicIdentifiedData &data);
	//@}

	/// @name data access (read / verify)
	//@{
	//@}

	/// @name save / write
	//@{
	/// @brief ファイルをセーブする前の準備を行う
	bool	PrepareToSaveFile(wxInputStream &istream, int &file_size, const DiskBasicDirItem *pitem, DiskBasicDirItem *nitem, DiskBasicError &errinfo);
	/// @brief データの書き込み処理
	int		WriteFile(DiskBasicDirItem *item, wxInputStream &istream, wxUint8 *buffer, int size, int remain, int sector_num, wxUint32 group_num, wxUint32 next_group, int sector_end);
	//@}

	/// @name delete
	//@{
	/// @brief ファイル削除後の処理
	bool	AdditionalProcessOnDeletedFile(DiskBasicDirItem *item);
	//@}
};

#endif /* _BASICTYPE_SDOS_H_ */
