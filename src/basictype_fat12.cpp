﻿/// @file basictype_fat12.cpp
///
/// @brief disk basic fat type
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "basictype_fat12.h"
#include "basicfmt.h"
#include "basicdir.h"
#include "basicdiritem.h"
#include "logging.h"


//
//
//
DiskBasicTypeFAT12::DiskBasicTypeFAT12(DiskBasic *basic, DiskBasicFat *fat, DiskBasicDir *dir)
	: DiskBasicType(basic, fat, dir)
{
}

/// ディスクから各パラメータを取得＆必要なパラメータを計算
/// @param [in] disk          ディスク
/// @param [in] is_formatting フォーマット中か
/// @retval 1.0>      正常
/// @retval 0.0 - 1.0 警告あり
/// @retval <0.0      エラーあり
double DiskBasicTypeFAT12::ParseParamOnDisk(DiskD88Disk *disk, bool is_formatting)
{
	if (is_formatting) return 1.0;

	int nums = 0;
	int valids = 0;

	// MS-DOS ディスク上のパラメータを読む
	DiskD88Sector *sector = disk->GetSector(0, 0, 1);
	if (!sector) return -1.0;
	wxUint8 *datas = sector->GetSectorBuffer();
	if (!datas) return -1.0;
	fat_bpb_t *bpb = (fat_bpb_t *)datas;

	nums++;
	if (bpb->BPB_SecPerClus != 0) {
		// クラスタサイズ
		valids++;
	}
	nums++;
	if (disk->GetSectorSize() == wxUINT16_SWAP_ON_BE(bpb->BPB_BytsPerSec)) {
		// セクタサイズ
		valids++;
	}
	nums++;
	if (disk->GetSidesPerDisk() >= wxUINT16_SWAP_ON_BE(bpb->BPB_NumHeads)) {
		// サイド数
		valids++;
	}
	nums++;
	if (basic->GetSidesPerDiskOnBasic() == wxUINT16_SWAP_ON_BE(bpb->BPB_NumHeads)) {
		// BASICテンプレートのサイド数
		valids++;
	}
	nums++;
	if (basic->GetSectorsPerTrackOnBasic() == wxUINT16_SWAP_ON_BE(bpb->BPB_SecPerTrk)) {
		// BASICテンプレートのセクタ数
		valids++;
	}

	if (nums == valids) {
		basic->SetSidesPerDiskOnBasic(wxUINT16_SWAP_ON_BE(bpb->BPB_NumHeads));
		basic->SetSectorsPerGroup(bpb->BPB_SecPerClus);
		basic->SetReservedSectors(wxUINT16_SWAP_ON_BE(bpb->BPB_RsvdSecCnt));
		basic->SetNumberOfFats(bpb->BPB_NumFATs);
		basic->SetSectorsPerFat(wxUINT16_SWAP_ON_BE(bpb->BPB_FATSz16));
		basic->SetDirEntryCount(wxUINT16_SWAP_ON_BE(bpb->BPB_RootEntCnt));

		basic->SetDirStartSector(-1);
		basic->SetDirEndSector(-1);
		basic->CalcDirStartEndSector(wxUINT16_SWAP_ON_BE(bpb->BPB_BytsPerSec));

		basic->SetSectorsPerTrackOnBasic(wxUINT16_SWAP_ON_BE(bpb->BPB_SecPerTrk));

		basic->SetMediaId(bpb->BPB_Media);

		// トラック数
		int tracks_per_side = wxUINT16_SWAP_ON_BE(bpb->BPB_TotSec16);
		if (tracks_per_side > 0) {
			tracks_per_side = tracks_per_side / basic->GetSidesPerDiskOnBasic() / basic->GetSectorsPerTrackOnBasic();
			basic->SetTracksPerSideOnBasic(tracks_per_side);
		}
	}

	// 最終グループ番号を計算
	int max_grp_on_fat = basic->GetSectorsPerFat() * basic->GetSectorSize() * 2 / 3;
//	int max_grp_on_prm = ((basic->GetSidesPerDiskOnBasic() * disk->GetTracksPerSide() * basic->GetSectorsPerTrackOnBasic() - basic->GetDirEndSector()) / basic->GetSectorsPerGroup());
	int max_grp_on_prm = (basic->GetSidesPerDiskOnBasic() * basic->GetTracksPerSide() * basic->GetSectorsPerTrackOnBasic() - basic->GetDirEndSector()) / basic->GetSectorsPerGroup() + 1;

	basic->SetFatEndGroup(max_grp_on_fat > max_grp_on_prm ? max_grp_on_prm : max_grp_on_fat);

	// テンプレートに一致するものがあるか
	const DiskBasicParam *param = gDiskBasicTemplates.FindType(basic->GetBasicCategoryName(), basic->GetBasicTypeName(), basic->GetSidesPerDiskOnBasic(), basic->GetSectorsPerTrackOnBasic());
	if (param) {
		basic->SetBasicDescription(param->GetBasicDescription());
	}

	double valid_ratio = 0.0;
	if (nums > 0) {
		valid_ratio = (double)valids/nums;
	}
	return valid_ratio;
}

/// FATエリアをチェック
/// @param [in] is_formatting フォーマット中か
/// @retval 1.0  正常
/// @retval <1.0 警告あり
/// @retval <0.0 エラーあり
double DiskBasicTypeFAT12::CheckFat(bool is_formatting)
{
	wxUint32 end = basic->GetFatEndGroup() < 0x1ff ? basic->GetFatEndGroup() : 0x1ff;
	wxUint8 *tbl = new wxUint8[end + 1];
	memset(tbl, 0, end + 1);

	// 同じグループ番号が重複しているか
	for(wxUint32 pos = 0; pos <= end; pos++) {
		wxUint32 gnum = GetGroupNumber(pos);
		if (gnum > 1 && gnum <= end) {
			tbl[gnum]++;
		}
	}
	// 同じグループ番号が重複している場合エラー
	double valid_ratio = 1.0;
	for(wxUint32 pos = 0; pos <= end; pos++) {
		if (tbl[pos] > 4) {
			valid_ratio = -1.0;
			break;
		}
	}
	delete [] tbl;

	if (valid_ratio < 0.0) return valid_ratio;

	// FATエリアの先頭がメディアIDであること
	int match = 0;
	DiskBasicFatArea *bufs = fat->GetDiskBasicFatArea();
	match = bufs->MatchData8(0, basic->GetMediaId());
	if (match != (basic->GetValidNumberOfFats() > 0 ? basic->GetValidNumberOfFats() : basic->GetNumberOfFats())) {
		valid_ratio -= 0.8;
	}

	// 最終グループ番号を計算
	int max_grp_on_fat = basic->GetSectorsPerFat() * basic->GetSectorSize() * 2 / 3;
	int max_grp_on_prm = (basic->GetSidesPerDiskOnBasic() * basic->GetTracksPerSide() * basic->GetSectorsPerTrackOnBasic() - basic->GetDirEndSector()) / basic->GetSectorsPerGroup() + 1;

	basic->SetFatEndGroup(max_grp_on_fat > max_grp_on_prm ? max_grp_on_prm : max_grp_on_fat);

	return valid_ratio;
}

/// 管理エリアのトラック番号からグループ番号を計算
wxUint32 DiskBasicTypeFAT12::CalcManagedStartGroup()
{
	managed_start_group = 0;
	return managed_start_group;
}

/// 使用可能なディスクサイズを得る
void DiskBasicTypeFAT12::GetUsableDiskSize(int &disk_size, int &group_size) const
{
	group_size = basic->GetFatEndGroup() - 2;
	disk_size = group_size * basic->GetSectorSize() * basic->GetSectorsPerGroup();
}

/// 残りディスクサイズを計算
void DiskBasicTypeFAT12::CalcDiskFreeSize(bool wrote)
{
	wxUint32 fsize = 0;
	wxUint32 grps = 0; 
	fat_availability.Empty();
	fat_availability.Add(FAT_AVAIL_SYSTEM);	// 0
	fat_availability.Add(FAT_AVAIL_SYSTEM);	// 1

	// クラスタは2から始まる
	for(wxUint32 pos = 2; pos <= basic->GetFatEndGroup(); pos++) {
		wxUint32 gnum = GetGroupNumber(pos);
		int fsts = FAT_AVAIL_USED;
		if (gnum == basic->GetGroupUnusedCode()) {
			fsize += (basic->GetSectorSize() * basic->GetSectorsPerGroup());
			grps++;
			fsts = FAT_AVAIL_FREE;
		} else if (gnum >= 0xff8) {
			fsts = FAT_AVAIL_USED_LAST;
		}
		fat_availability.Add(fsts);
//		myLog.SetDebug("DiskBasicTypeFAT12::CalcDiskFreeSize: pos:%d gnum:%d size:%d grps:%d", pos, gnum, fsize, grps);
	}

	free_disk_size = (int)fsize;
	free_groups = (int)grps;
}

/// FAT位置をセット
/// @param [in] num グループ番号(0...)
/// @param [in] val 値
void DiskBasicTypeFAT12::SetGroupNumber(wxUint32 num, wxUint32 val)
{
	fat->GetDiskBasicFatArea()->SetData12LE(num, val);
}

/// FAT位置を返す
/// @param [in] num グループ番号(0...)
wxUint32 DiskBasicTypeFAT12::GetGroupNumber(wxUint32 num) const
{
	wxUint32 new_num = fat->GetDiskBasicFatArea()->GetData12LE(0, num);
	return new_num;
}

/// 次の空き位置を返す
/// @return INVALID_GROUP_NUMBER: 空きなし
wxUint32 DiskBasicTypeFAT12::GetNextEmptyGroupNumber(wxUint32 curr_group)
{
	wxUint32 new_num = INVALID_GROUP_NUMBER;

	// グループが連続するように検索
	int group_start = curr_group;
	int group_end = basic->GetFatEndGroup();
	bool found = false;

	for(int i=0; i<2; i++) {
		for(int g = group_start; g <= group_end; g++) {
			wxUint32 gnum = GetGroupNumber(g);
//			myLog.SetDebug("DiskBasicTypeFAT12::GetNextEmptyGroupNumber: g:%d gnum:%d", g, gnum);
			if (gnum == basic->GetGroupUnusedCode()) {
				new_num = g;
				found = true;
				break;
			}
		}
		if (found) break;
		// ないときは最初からさがす
		group_start = 2;
	}
	return new_num;
}

/// グループ番号からセクタ番号を得る
int DiskBasicTypeFAT12::GetStartSectorFromGroup(wxUint32 group_num)
{
	// 2から始まる (0,1は予約)
	if (group_num < 2) {
		return -1;
	}
	return (group_num - 2) * basic->GetSectorsPerGroup();
}

/// グループ番号から最終セクタ番号を得る
int DiskBasicTypeFAT12::GetEndSectorFromGroup(wxUint32 group_num, wxUint32 next_group, int sector_start, int sector_size, int remain_size)
{
	int sector_end = sector_start + basic->GetSectorsPerGroup() - 1;
	return sector_end;
}

/// データ領域の開始セクタを計算
int DiskBasicTypeFAT12::CalcDataStartSectorPos()
{
	return basic->GetDirEndSector();	// 0始まりで計算する
}

/// セクタデータを指定コードで埋める
void DiskBasicTypeFAT12::FillSector(DiskD88Track *track, DiskD88Sector *sector)
{
	int sector_pos = basic->CalcSectorPosFromNumForGroup(track->GetTrackNumber(), track->GetSideNumber(), sector->GetSectorNumber());
	if (sector_pos < 0) {
		// ファイル管理エリアの場合
		sector->Fill(basic->GetFillCodeOnFAT());
	} else {
		// ユーザーエリア
		sector->Fill(basic->GetFillCodeOnFormat());
	}
}

/// セクタデータを埋めた後の個別処理
/// ヘッダのパラメータを設定
bool DiskBasicTypeFAT12::AdditionalProcessOnFormatted(const DiskBasicIdentifiedData &data)
{
	if (!CreateBiosParameterBlock("\xeb\x3c\x90", "FAT12")) {
		return false;
	}
	return true;
}

/// BIOS Parameter Block を作成
bool DiskBasicTypeFAT12::CreateBiosParameterBlock(const char *jmp, const char *name, wxUint8 **sec_buf)
{
	DiskD88Sector *sec = basic->GetSector(0, 0, 1);
	if (!sec) return false;
	wxUint8 *buf = sec->GetSectorBuffer();
	if (!buf) return false;

	if(sec_buf) *sec_buf = buf;

	sec->Fill(0);

	fat_bpb_t *hed = (fat_bpb_t *)buf;

	size_t len;

	wxCharBuffer s_jmp = basic->GetVariousStringParam(wxT("JumpBoot")).To8BitData();
	if (s_jmp.length() > 0) {
		jmp = s_jmp.data();
	}
	len = strlen(jmp) < sizeof(hed->BS_JmpBoot) ? strlen(jmp) : sizeof(hed->BS_JmpBoot);
	memcpy(hed->BS_JmpBoot, jmp, len);

	hed->BPB_BytsPerSec = wxUINT16_SWAP_ON_BE(basic->GetSectorSize());
	hed->BPB_SecPerClus = basic->GetSectorsPerGroup();
	hed->BPB_RsvdSecCnt = wxUINT16_SWAP_ON_BE(basic->GetReservedSectors());
	hed->BPB_NumFATs = basic->GetNumberOfFats();
	hed->BPB_RootEntCnt = wxUINT16_SWAP_ON_BE(basic->GetDirEntryCount());

	wxCharBuffer s_name = basic->GetVariousStringParam(wxT("OEMName")).To8BitData();
	if (s_name.length() > 0) {
		name = s_name.data();
	}
	// 上記パラメータ領域をまたがって設定可能にする
	len = strlen(name) < 16 ? strlen(name) : 16;
	memset(hed->BS_OEMName, 0x20, sizeof(hed->BS_OEMName));
	memcpy(hed->BS_OEMName, name, len);

	len = basic->GetTracksPerSide() * basic->GetSectorsPerTrackOnBasic() * basic->GetSidesPerDiskOnBasic();
	hed->BPB_TotSec16 = wxUINT16_SWAP_ON_BE(len);
	hed->BPB_Media = basic->GetMediaId();
	hed->BPB_FATSz16 = wxUINT16_SWAP_ON_BE(basic->GetSectorsPerFat());
	hed->BPB_SecPerTrk =  wxUINT16_SWAP_ON_BE(basic->GetSectorsPerTrackOnBasic());
	hed->BPB_NumHeads = wxUINT16_SWAP_ON_BE(basic->GetSidesPerDiskOnBasic());

	// FATの先頭にメディアIDをセット
	SetGroupNumber(0, 0xffffff00 | basic->GetMediaId());
	SetGroupNumber(1, 0xffffffff);

	return true;
}

/// グループ確保時に最後のグループ番号を計算する
/// @param [in]     group_num	現在のグループ番号
/// @param [in,out] size_remain	残りのデータサイズ
/// @return 最後のグループ番号
wxUint32 DiskBasicTypeFAT12::CalcLastGroupNumber(wxUint32 group_num, int &size_remain)
{
	return (group_num != INVALID_GROUP_NUMBER ? basic->GetGroupFinalCode() : group_num);
}

/// ルートディレクトリか
bool DiskBasicTypeFAT12::IsRootDirectory(wxUint32 group_num)
{
	// オフセット未満だったらルート
	return (group_num <= 1);
}

/// サブディレクトリを作成する前にディレクトリ名を編集する
bool DiskBasicTypeFAT12::RenameOnMakingDirectory(wxString &dir_name)
{
	// 空や"."で始まるディレクトリは作成不可
	if (dir_name.IsEmpty() || dir_name.Left(1) == wxT(".")) {
		return false;
	}
	return true;
}

/// サブディレクトリを作成した後の個別処理
void DiskBasicTypeFAT12::AdditionalProcessOnMadeDirectory(DiskBasicDirItem *item, DiskBasicGroups &group_items, const DiskBasicDirItem *parent_item)
{
	if (group_items.Count() <= 0) return;

	// ファイルサイズをクリア
	item->SetFileSize(0);

	// カレントと親ディレクトリのエントリを作成する
	DiskBasicGroupItem *gitem = &group_items.Item(0);

	DiskD88Sector *sector = basic->GetDisk()->GetSector(gitem->track, gitem->side, gitem->sector_start);

	wxUint8 *buf = sector->GetSectorBuffer();
	DiskBasicDirItem *newitem = basic->CreateDirItem(sector, 0, buf);

	// カレント
	newitem->CopyData(item->GetData());
	newitem->SetFileNamePlain(wxT("."));
	newitem->SetFileAttr(FORMAT_TYPE_UNKNOWN, FILE_TYPE_DIRECTORY_MASK);

	// 親
	buf += newitem->GetDataSize();
	newitem->SetDataPtr(0, 0, 0, sector, 0, buf);
	if (parent_item) {
		// 親がサブディレクトリ
		newitem->CopyData(parent_item->GetData());
	} else {
		// 親がルート
		newitem->CopyData(item->GetData());
		newitem->SetStartGroup(0, 0);
	}
	struct tm tm;
	item->GetFileDateTime(&tm);
	newitem->SetFileDateTime(&tm);
	newitem->SetFileNamePlain(wxT(".."));
	newitem->SetFileAttr(FORMAT_TYPE_UNKNOWN, FILE_TYPE_DIRECTORY_MASK);

	delete newitem;
}
