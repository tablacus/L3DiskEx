﻿/// @file basictype_l32d.h
///
/// @brief disk basic fat type
///
#ifndef _BASICTYPE_L32D_H_
#define _BASICTYPE_L32D_H_

#include "common.h"
#include "basiccommon.h"
#include "basictype_fat8.h"

/// LEVEL-3 BASIC 2Dの処理
class DiskBasicTypeL32D : public DiskBasicTypeFAT8
{
private:
	DiskBasicTypeL32D() : DiskBasicTypeFAT8() {}
	DiskBasicTypeL32D(const DiskBasicType &src) : DiskBasicTypeFAT8(src) {}
public:
	DiskBasicTypeL32D(DiskBasic *basic, DiskBasicFat *fat, DiskBasicDir *dir);

	/// @name access to FAT area
	//@{
	/// 空きFAT位置を返す
	wxUint32	GetEmptyGroupNumber();
	//@}
	/// @name check / assign FAT area
	//@{
	/// 管理エリアのトラック番号からグループ番号を計算
	wxUint32	CalcManagedStartGroup();
	//@}

	/// @name file chain
	//@{
	/// スキップするトラック番号
	int		CalcSkippedTrack();
	/// データ領域の開始セクタを計算
	int		CalcDataStartSectorPos();
	//@}
};

#endif /* _BASICTYPE_L32D_H_ */