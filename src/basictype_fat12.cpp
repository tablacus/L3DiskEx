﻿/// @file basictype_fat12.cpp
///
/// @brief disk basic fat type
///
#include "basictype_fat12.h"
#include "basicfmt.h"
#include "basicdiritem.h"
#include "logging.h"

//
//
//
DiskBasicTypeFAT12::DiskBasicTypeFAT12(DiskBasic *basic, DiskBasicFat *fat, DiskBasicDir *dir)
	: DiskBasicType(basic, fat, dir)
{
}

/// FATエリアをチェック
bool DiskBasicTypeFAT12::CheckFat()
{
	bool valid = true;

	wxUint32 end = end_group < 0x1ff ? end_group : 0x1ff;
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
	for(wxUint32 pos = 0; pos <= end; pos++) {
		if (tbl[pos] > 4) {
			valid = false;
			break;
		}
	}
	delete [] tbl;

	// FATエリアの先頭がメディアIDであること
	int match = 0;
	DiskBasicFatArea *bufs = fat->GetDiskBasicFatArea();
	for(size_t j=0; j<bufs->Count(); j++) {
		DiskBasicFatBuffers *fatbufs = &bufs->Item(j);
		DiskBasicFatBuffer *fatbuf = &fatbufs->Item(0);
		if (fatbuf->buffer[0] == basic->GetMediaId()) {
			match++;
		}
	}
	if (match != basic->GetNumberOfFats()) {
		valid = false;
	}

	// 最終グループ番号を計算
	int max_grp_on_fat = basic->GetSectorsPerFat() * basic->GetSectorSize() * 2 / 3;
	int max_grp_on_prm = (basic->GetSidesOnBasic() * basic->GetTracksPerSide() * basic->GetSectorsPerTrack() - basic->GetDirEndSector()) / basic->GetSectorsPerGroup() + 1;

	end_group = max_grp_on_fat > max_grp_on_prm ? max_grp_on_prm : max_grp_on_fat;
	basic->SetFatEndGroup(end_group);

	return valid;
}

/// ディスク上のパラメータを読む
bool DiskBasicTypeFAT12::ParseParamOnDisk(DiskD88Disk *disk)
{
	bool valid = true;

	// MS-DOS ディスク上のパラメータを読む
	DiskD88Sector *sector = disk->GetSector(0, 0, 1);
	wxUint8 *datas = sector->GetSectorBuffer();
	fat_bpb_t *bpb = (fat_bpb_t *)datas;
	if (bpb->BPB_SecPerClus == 0) {
		// クラスタサイズがおかしい
		valid = false;
	}
	if (disk->GetSectorSize() != wxUINT16_SWAP_ON_BE(bpb->BPB_BytsPerSec)) {
		// セクタサイズが違う
		valid = false;
	}
	if (disk->GetSidesPerDisk() < wxUINT16_SWAP_ON_BE(bpb->BPB_NumHeads)) {
		// サイド数が少ない
		valid = false;
	}

	if (valid) {
		secs_per_group = bpb->BPB_SecPerClus;
//		grps_per_track = wxUINT16_SWAP_ON_BE(bpb->BPB_SecPerTrk) / secs_per_group;

		basic->SetSidesOnBasic(wxUINT16_SWAP_ON_BE(bpb->BPB_NumHeads));
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

		// 最終グループ番号を計算
		int max_grp_on_fat = basic->GetSectorsPerFat() * basic->GetSectorSize() * 2 / 3;
		int max_grp_on_prm = ((wxUINT16_SWAP_ON_BE(bpb->BPB_NumHeads) * disk->GetTracksPerSide() * wxUINT16_SWAP_ON_BE(bpb->BPB_SecPerTrk) - basic->GetDirEndSector()) / bpb->BPB_SecPerClus);

		end_group = max_grp_on_fat > max_grp_on_prm ? max_grp_on_prm : max_grp_on_fat;
		basic->SetFatEndGroup(end_group);

		// テンプレートに一致するものがあるか
		DiskBasicParam *param = gDiskBasicTemplates.FindType(basic->GetBasicCategoryName(), wxUINT16_SWAP_ON_BE(bpb->BPB_NumHeads), wxUINT16_SWAP_ON_BE(bpb->BPB_SecPerTrk));
		if (param) {
			basic->SetBasicDescription(param->GetBasicDescription());
		}
	}

	return valid;
}

/// 管理エリアのトラック番号からグループ番号を計算
wxUint32 DiskBasicTypeFAT12::CalcManagedStartGroup()
{
	managed_start_group = 0;
	return managed_start_group;
}

/// 残りディスクサイズを計算
void DiskBasicTypeFAT12::CalcDiskFreeSize()
{
	wxUint32 fsize = 0;
	wxUint32 grps = 0; 
	fat_availability.Empty();
	fat_availability.Add(FAT_AVAIL_SYSTEM);	// 0
	fat_availability.Add(FAT_AVAIL_SYSTEM);	// 1

	// クラスタは2から始まる
	for(wxUint32 pos = 2; pos <= end_group; pos++) {
		wxUint32 gnum = GetGroupNumber(pos);
		int fsts = FAT_AVAIL_USED;
		if (gnum == group_unused_code) {
			fsize += (sector_size * secs_per_group);
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
	DiskBasicFatArea *bufs = fat->GetDiskBasicFatArea();
	for(size_t j=0; j<bufs->Count(); j++) {
		DiskBasicFatBuffers *fatbufs = &bufs->Item(j);
		// 12bit FAT
		wxUint32 pos = ((num >> 1) * 3 + (num & 1));
		int next = 0;
		for(size_t i=0; i<fatbufs->Count(); i++) {
			DiskBasicFatBuffer *fatbuf = &fatbufs->Item(i);
			if (next == 0 && pos < (wxUint32)fatbuf->size) {
				wxUint8 dat = fatbuf->buffer[pos];
				if (num & 1) {
					// odd
					dat = (dat & 0x0f) | ((val & 0x0f) << 4);
				} else {
					// even
					dat = val & 0xff;
				}
				fatbuf->buffer[pos] = dat;
				pos++;
				next = 1;
			}
			if (next == 1 && pos < (wxUint32)fatbuf->size) {
				wxUint8 dat = fatbuf->buffer[pos];
				if (num & 1) {
					// odd
					dat = ((val >> 4) & 0xff);
				} else {
					// even
					dat = (dat & 0xf0) | ((val >> 8) & 0x0f);
				}
				fatbuf->buffer[pos] = dat;
				break;
			}
			pos -= fatbuf->size;
		}
	}
}

/// FAT位置を返す
/// @param [in] num グループ番号(0...)
wxUint32 DiskBasicTypeFAT12::GetGroupNumber(wxUint32 num)
{
	wxUint32 new_num = 0;
	DiskBasicFatArea *bufs = fat->GetDiskBasicFatArea();
	DiskBasicFatBuffers *fatbufs = &bufs->Item(0);
	// 12bit FAT
	wxUint32 pos = ((num >> 1) * 3 + (num & 1));
	int next = 0;
	for(size_t i=0; i<fatbufs->Count(); i++) {
		DiskBasicFatBuffer *fatbuf = &fatbufs->Item(i);
		if (next == 0 && pos < (wxUint32)fatbuf->size) {
			if (num & 1) {
				// odd
				new_num = ((fatbuf->buffer[pos] & 0xf0) >> 4);
			} else {
				// even
				new_num = fatbuf->buffer[pos];
			}
			pos++;
			next = 1;
		}
		if (next == 1 && pos < (wxUint32)fatbuf->size) {
			if (num & 1) {
				// odd
				new_num |= ((wxUint32)fatbuf->buffer[pos] << 4);
			} else {
				// even
				new_num |= ((fatbuf->buffer[pos] & 0x0f) << 8);
			}
			break;
		}
		pos -= fatbuf->size;
	}
	return new_num;
}

/// 次の空き位置を返す
/// @return INVALID_GROUP_NUMBER: 空きなし
wxUint32 DiskBasicTypeFAT12::GetNextEmptyGroupNumber(wxUint32 curr_group)
{
	wxUint32 new_num = INVALID_GROUP_NUMBER;

	// グループが連続するように検索
	int group_start = curr_group;
	int group_end = end_group;
	bool found = false;

	for(int i=0; i<2; i++) {
		for(int g = group_start; g <= group_end; g++) {
			wxUint32 gnum = GetGroupNumber(g);
//			myLog.SetDebug("DiskBasicTypeFAT12::GetNextEmptyGroupNumber: g:%d gnum:%d", g, gnum);
			if (gnum == group_unused_code) {
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
	return (group_num - 2) * secs_per_group;
}

/// グループ番号から最終セクタ番号を得る
int DiskBasicTypeFAT12::GetEndSectorFromGroup(wxUint32 group_num, wxUint32 next_group, int sector_start, int sector_size, int remain_size)
{
	int sector_end = sector_start + secs_per_group - 1;
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
void DiskBasicTypeFAT12::AdditionalProcessOnFormatted()
{
	fat_bpb_t hed;
	memset(&hed, 0, sizeof(hed));
	memcpy(hed.BS_JmpBoot, "\xeb\x3c\x90", 3);
	memcpy(hed.BS_OEMName, "FAT12   ", 8);
	hed.BPB_BytsPerSec = wxUINT16_SWAP_ON_BE(basic->GetSectorSize());
	hed.BPB_SecPerClus = basic->GetSectorsPerGroup();
	hed.BPB_RsvdSecCnt = wxUINT16_SWAP_ON_BE(basic->GetReservedSectors());
	hed.BPB_NumFATs = basic->GetNumberOfFats();
	hed.BPB_RootEntCnt = wxUINT16_SWAP_ON_BE(basic->GetDirEntryCount());
	hed.BPB_TotSec16 = basic->GetTracksPerSide() * basic->GetSectorsPerTrackOnBasic() * basic->GetSidesOnBasic();
	hed.BPB_TotSec16 = wxUINT16_SWAP_ON_BE(hed.BPB_TotSec16);
	hed.BPB_Media = basic->GetMediaId();
	hed.BPB_FATSz16 = wxUINT16_SWAP_ON_BE(basic->GetSectorsPerFat());
	hed.BPB_SecPerTrk =  wxUINT16_SWAP_ON_BE(basic->GetSectorsPerTrackOnBasic());
	hed.BPB_NumHeads = wxUINT16_SWAP_ON_BE(basic->GetSidesOnBasic());

	DiskD88Sector *sec = basic->GetDisk()->GetSector(0, 0, 1);
	if (!sec) return;
	wxUint8 *buf = sec->GetSectorBuffer();
	if (!buf) return;
	memcpy(buf, &hed, sizeof(hed));

	// FATの先頭にメディアIDをセット
	SetGroupNumber(0, 0xffffff00 | basic->GetMediaId());
	SetGroupNumber(1, 0xffffffff);
}

/// 最後のグループ番号を計算する
wxUint32 DiskBasicTypeFAT12::CalcLastGroupNumber(wxUint32 group_num, int size_remain)
{
	return (group_num != INVALID_GROUP_NUMBER ? group_final_code : group_num);
}

/// ルートディレクトリか
bool DiskBasicTypeFAT12::IsRootDirectory(wxUint32 group_num)
{
	// オフセット未満だったらルート
	return (group_num <= 1);
}

/// サブディレクトリを作成した後の個別処理
void DiskBasicTypeFAT12::AdditionalProcessOnMadeDirectory(DiskBasicDirItem *item, DiskBasicGroups &group_items, const DiskBasicDirItem *parent_item, wxUint32 parent_group_num)
{
	if (group_items.Count() <= 0) return;

	// カレントと親ディレクトリのエントリを作成する
	DiskBasicGroupItem *gitem = &group_items.Item(0);

	DiskD88Sector *sector = basic->GetDisk()->GetSector(gitem->track, gitem->side, gitem->sector_start);

	wxUint8 *buf = sector->GetSectorBuffer();
	DiskBasicDirItem *newitem = basic->CreateDirItem(sector, buf);

	// カレント
	newitem->CopyData(item->GetData());
	newitem->SetFileNamePlain(wxT("."));
	newitem->SetFileAttr(FILE_TYPE_DIRECTORY_MASK);

	buf += newitem->GetDataSize();
	newitem->SetDataPtr((directory_t *)buf);

	// 親
	if (parent_item) {
		// 親がサブディレクトリ
		newitem->CopyData(parent_item->GetData());
	} else {
		// 親がルート
		newitem->CopyData(item->GetData());
		newitem->SetStartGroup(parent_group_num);
	}
	newitem->SetFileNamePlain(wxT(".."));
	newitem->SetFileAttr(FILE_TYPE_DIRECTORY_MASK);

	delete newitem;
}