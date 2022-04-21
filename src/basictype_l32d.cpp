﻿/// @file basictype_l32d.cpp
///
/// @brief disk basic fat type for L3 BASIC 2D
///
#include "basictype_l32d.h"
#include "basicfmt.h"

//
//
//
DiskBasicTypeL32D::DiskBasicTypeL32D(DiskBasic *basic, DiskBasicFat *fat, DiskBasicDir *dir)
	: DiskBasicTypeFAT8(basic, fat, dir)
{
}

/// 空き位置を返す
/// @return 0xff:空きなし
wxUint32 DiskBasicTypeL32D::GetEmptyGroupNumber()
{
	wxUint32 new_num = INVALID_GROUP_NUMBER;
	// 管理エリアに近い位置から検索

	// トラック当たりのグループ数
	wxUint32 grps_per_trk = basic->GetSectorsPerTrackOnBasic() * 2 / basic->GetSectorsPerGroup();
	// 最大グループ数
	wxUint32 max_group = end_group + 1 - managed_start_group;
	if (max_group < managed_start_group) max_group = managed_start_group;
	max_group = max_group * 2 - 1;

	for(wxUint32 i = 0; i <= max_group; i++) {
		int i2 = (i / grps_per_trk);
		int i4 = (i / grps_per_trk / 2);
		wxUint32 num;
		if ((i2 & 1) == 0) {
			num = managed_start_group - ((i4 + 1) * grps_per_trk) + (i % grps_per_trk);
		} else {
			num = managed_start_group + (i4 * grps_per_trk) + (i % grps_per_trk);
		}
		if (end_group < num) {
			continue;
		}
		wxUint32 gnum = GetGroupNumber(num);
		if (gnum == group_unused_code) {
			new_num = num;
			break;
		}
	}
	return new_num;
}

/// 管理エリアのトラック番号からグループ番号を計算
wxUint32 DiskBasicTypeL32D::CalcManagedStartGroup()
{
	int trk = basic->GetManagedTrackNumber();
	int sid = basic->GetFatSideNumber();
	int sides = basic->GetSidesOnBasic();
	int secs_per_grp = basic->GetSectorsPerGroup();
	int secs_per_trk = basic->GetSectorsOnBasic();
	// トラック1から開始するので-1する
	trk--;
	managed_start_group = (trk * sides + sid) * secs_per_trk / secs_per_grp;
	return managed_start_group;
}

/// データ領域の開始セクタを計算
int DiskBasicTypeL32D::CalcDataStartSectorPos()
{
	// トラック0を除く
	return basic->GetSectorsPerTrackOnBasic() * basic->GetSidesOnBasic();
}

/// スキップするトラック番号
int DiskBasicTypeL32D::CalcSkippedTrack()
{
	return basic->GetManagedTrackNumber();
}