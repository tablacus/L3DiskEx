﻿/// @file basictype_msdos.cpp
///
/// @brief disk basic fat type for MS-DOS
///
#include "basictype_msdos.h"
#include "basicfmt.h"
#include "basicdir.h"

//
//
//
DiskBasicTypeMSDOS::DiskBasicTypeMSDOS(DiskBasic *basic, DiskBasicFat *fat, DiskBasicDir *dir)
	: DiskBasicTypeFAT12(basic, fat, dir)
{
}

/// セクタデータを埋めた後の個別処理
/// フォーマット IPLの書き込み
bool DiskBasicTypeMSDOS::AdditionalProcessOnFormatted()
{
	if (!DiskBasicTypeFAT12::AdditionalProcessOnFormatted()) {
		return false;
	}

	// ボリュームラベルを設定
	int dir_start = basic->GetReservedSectors() + basic->GetNumberOfFats() * basic->GetSectorsPerFat();
	DiskD88Sector *sec = basic->GetSectorFromSectorPos(dir_start);
	DiskBasicDirItem *ditem = dir->NewItem(sec, sec->GetSectorBuffer());

	ditem->SetFileNamePlain(wxT("MS-DOS"));
	ditem->SetFileAttr(FILE_TYPE_VOLUME_MASK);
	struct tm tm;
	wxDateTime::GetTmNow(&tm);
	ditem->SetFileDateTime(&tm);

	delete ditem;

	return true;
}