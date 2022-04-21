﻿/// @file basicerror.h
///
/// @brief disk basic error messages
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef _BASICERROR_H_
#define _BASICERROR_H_

#include "common.h"
#include "result.h"


extern const char *gDiskBasicErrorMsgs[];

/// エラー情報
class DiskBasicError : public ResultInfo
{
public:
	DiskBasicError() : ResultInfo() {}

	enum {
		// 引数なしメッセージ
		ERR_NONE = 0,
		ERR_SUPPORTED,
		ERR_FORMATTED,
		ERR_UNSELECT_DISK,
		ERR_WRITE_PROTECTED,
		ERR_WRITE_UNSUPPORTED,
		ERR_FILE_NOT_FOUND,
		ERR_FILE_ALREADY_EXIST,
		ERR_DIRECTORY_FULL,
		ERR_DISK_FULL,
		ERR_FILE_TOO_LARGE,
		ERR_NOT_ENOUGH_FREE,
		ERR_CANNOT_EXPORT,
		ERR_CANNOT_IMPORT,
		ERR_CANNOT_VERIFY,
		ERR_CANNOT_FORMAT,
		ERR_FORMATTING,
		ERR_FORMAT_UNSUPPORTED,
		ERR_DELETE_UNSUPPORTED,
		ERR_CANNOT_MAKE_DIRECTORY,
		ERR_MAKING_DIRECTORY,
		ERR_IN_FAT_AREA,
		ERR_IN_DIRECTORY_AREA,
		ERR_IN_PARAMETER_AREA,
		ERR_FILENAME_EMPTY,
		ERR_END_ADDR_TOO_SMALL,

		ERRV_START,
		// 引数あり（要フォーマット）のメッセージはこれ以降に設定
		ERRV_VERIFY_FILE,
		ERRV_MISMATCH_FILESIZE,
		ERRV_NO_TRACK,
		ERRV_NO_SECTOR,
		ERRV_INVALID_SECTOR,
		ERRV_NOTHING_IN_TRACK,
		ERRV_NUM_OF_SECTORS_IN_TRACK,
		ERRV_NO_SECTOR_IN_TRACK,
		ERRV_INVALID_VALUE_IN,
		ERRV_CANNOT_EDIT_ENAME,
		ERRV_CANNOT_DELETE,
		ERRV_CANNOT_DELETE_DIRECTORY,
		ERRV_ALREADY_DELETED,
		ERRV_ALREADY_EXISTS,
		ERRV_END
	};

	void SetMessage(int error_number, va_list ap);
};

#endif /* _BASICERROR_H_ */
