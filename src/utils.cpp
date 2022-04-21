﻿/// @file utils.cpp
///
/// @brief いろいろ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "utils.h"
#include <wx/translation.h>
#include <wx/string.h>
#include <wx/regex.h>


namespace Utils
{

//
//
//
TempData::TempData()
{
	alloc_size = TEMP_DATA_SIZE;
	data = new wxUint8[alloc_size];
	memset(data, 0, alloc_size);
	size = 0;
}

TempData::~TempData()
{
	delete data;
}

void TempData::SetSize(size_t val)
{
	if (val > alloc_size) {
		delete data;
		alloc_size = val;
		data = new wxUint8[alloc_size];
		memset(data, 0, alloc_size);
	}
	size = val; 
}

void TempData::SetData(const wxUint8 *data, size_t len, bool invert)
{
	SetSize(len);
	memcpy(this->data, data, this->size);
	if (invert) {
		mem_invert(this->data, this->size);
	}
}

void TempData::Set(size_t pos, wxUint8 val)
{
	if (pos < size) {
		data[pos] = val;
	}
}

/// 一致するバイトデータを置換
void TempData::Replace(wxUint8 src, wxUint8 dst)
{
	for(size_t pos=0; pos<size; pos++) {
		if (data[pos] == src) data[pos] = dst;
	}
}

void TempData::InvertData(bool invert)
{
	if (invert) {
		mem_invert(data, size);
	}
}

//
//
//

int Dump::Binary(const wxUint8 *buffer, size_t bufsize, wxString &str, bool invert)
{
	int rows = 0;
	wxUint8 inv = invert ? 0xff : 0;
	str += wxT("    :");
	for(size_t col = 0; col < 16; col++) {
		str += wxString::Format(wxT(" +%x"), (int)col);
	}
	str += wxT("\n");
	str += wxT("-----");
	for(size_t col = 0; col < 16; col++) {
		str += wxT("---");
	}
	str += wxT("\n");
	for(size_t pos = 0, col = 0; pos < bufsize; pos++) {
		if (col == 0) {
			str += wxString::Format(wxT("+%02x0:"), rows);
		}
		str += wxString::Format(wxT(" %02x"), (buffer[pos] ^ inv));
		if (col >= 15) {
			str += wxT("\n");
			col = 0;
			rows++;
		} else {
			col++;
		}
	}
	rows += 3;
	return rows;
}

int Dump::Ascii(const wxUint8 *buffer, size_t bufsize, int char_code, wxString &str, bool invert)
{
	wxUint16 inv = invert ? 0xff : 0;

	switch(char_code) {
	case 1:
		codes.SetMap(wxT("sjis"));
		break;
	default:
		codes.SetMap(wxT("hankaku"));
		break;
	}

	for(size_t col = 0; col < 16; col++) {
		str += wxString::Format(wxT("%x"), (int)col);
	}
	str += wxT("\n");
	for(size_t col = 0; col < 16; col++) {
		str += wxT("-");
	}
	str += wxT("\n");

	for(size_t pos = 0, col = 0; pos < bufsize; ) {
		if (col >= 16) {
			str += wxT("\n");
			col -= 16;
			if (col > 0) {
				str += wxString(" ", (size_t)col);
			}
		}

		wxString cstr;
		wxUint8 c[4];
		c[0] = buffer[pos] ^ inv;
		c[1] = pos + 1 == bufsize ? 0 : buffer[pos + 1] ^ inv;
		c[2] = 0;

		size_t len = codes.FindString(c, 2, cstr, '.');
		str += cstr;
		pos += len;
		col += len;
	}
	str += wxT("\n");
	return 0;
}

int Dump::Text(const wxUint8 *buffer, size_t bufsize, int char_code, wxString &str, bool invert)
{
	wxUint16 inv = invert ? 0xff : 0;

	switch(char_code) {
	case 1:
		codes.SetMap(wxT("sjis"));
		break;
	default:
		codes.SetMap(wxT("hankaku"));
		break;
	}

	int col = 0;
	int row = 1;
	for(size_t pos = 0; pos < bufsize; ) {
		wxString cstr;
		wxUint8 c[4];
		c[0] = buffer[pos] ^ inv;
		c[1] = pos + 1 == bufsize ? 0 : buffer[pos + 1] ^ inv;
		c[2] = 0;

		if (col >= 80) {
			row++;
			col=0;
		}

		if (c[0] == '\r' && c[1] == '\n') {
			// 改行
			str += wxT("\n");
			row++;
			col=0;
			pos+=2;
			continue;
		} else if (c[0] == '\r' || c[0] == '\n') {
			// 改行
			str += wxT("\n");
			row++;
			col=0;
			pos++;
			continue;
		} else if (c[0] == '\t') {
			// タブ
			if (c[1] >= 1 && c[1] < 0x20) {
				// for FLEX
				for(int i=0; i<c[1]; i++) {
					str += wxT(" ");
				}
				col+=(int)c[1];
				pos+=2;
			} else {
				str += wxT("\t");
				col+=8;
				pos++;
			}
			continue;
		} else if (c[0] < 0x20) {
			// コントロールコードは"."に変換
			str += wxT(".");
			col++;
			pos++;
			continue;
		}

		size_t len = codes.FindString(c, 2, cstr, '.');
		str += cstr;
		col += (int)len;
		pos += len;
	}
	return row;
}

void ConvTmToDateTime(const struct tm *tm, wxUint8 *date, wxUint8 *time)
{
	date[0] = (tm->tm_year & 0xff);
	date[1] = ((tm->tm_mon & 0x0f) << 4) | ((tm->tm_year & 0xf00) >> 8);
	date[2] = (tm->tm_mday & 0xff);

	time[0] = (tm->tm_hour & 0xff);
	time[1] = (tm->tm_min & 0xff);
	time[2] = (tm->tm_sec & 0xff);
}
void ConvDateTimeToTm(const wxUint8 *date, const wxUint8 *time, struct tm *tm)
{
	tm->tm_year = (int)date[0] | ((int)date[1] & 0xf) << 8;
	tm->tm_mon  = (date[1] & 0xf0) >> 4;
	if (tm->tm_mon == 0xf) tm->tm_mon = -1;
	tm->tm_mday = date[2];

	tm->tm_hour = time[0];
	tm->tm_min = time[1];
	tm->tm_sec = time[2];
}
bool ConvDateStrToTm(const wxString &date, struct tm *tm)
{
	wxRegEx re("^([0-9]+)[/:.-]([0-9]+)[/:.-]([0-9]+)$");
	wxString sval;
	long lval;
	bool valid = true;
	if (re.Matches(date)) {
		// year
		sval = re.GetMatch(date, 1);
		sval.ToLong(&lval);
		if (lval >= 1900) lval -= 1900;
		tm->tm_year = (int)lval;

		// month
		sval = re.GetMatch(date, 2);
		sval.ToLong(&lval);
		tm->tm_mon = (int)lval - 1;

		// day
		sval = re.GetMatch(date, 3);
		sval.ToLong(&lval);
		tm->tm_mday = (int)lval;
	} else {
		// invalid
		tm->tm_year = -1;
		tm->tm_mon = -2;
		tm->tm_mday = -1;

		valid = false;
	}
	return valid;
}
bool ConvTimeStrToTm(const wxString &time, struct tm *tm)
{
	wxRegEx re1("^([0-9]+)[/:.-]([0-9]+)[/:.-]([0-9]+)$");
	wxRegEx re2("^([0-9]+)[/:.-]([0-9]+)$");
	wxString sval;
	long lval;
	bool valid = true;
	if (re1.Matches(time)) {
		// hour
		sval = re1.GetMatch(time, 1);
		sval.ToLong(&lval);
		tm->tm_hour = (int)lval;

		// minute
		sval = re1.GetMatch(time, 2);
		sval.ToLong(&lval);
		tm->tm_min = (int)lval;

		// day
		sval = re1.GetMatch(time, 3);
		sval.ToLong(&lval);
		tm->tm_sec = (int)lval;

	} else if (re2.Matches(time)) {
		// hour
		sval = re2.GetMatch(time, 1);
		sval.ToLong(&lval);
		tm->tm_hour = (int)lval;

		// minute
		sval = re2.GetMatch(time, 2);
		sval.ToLong(&lval);
		tm->tm_min = (int)lval;
	} else {
		// invalid
		tm->tm_hour = -1;
		tm->tm_min = -1;
		tm->tm_sec = -1;

		valid = false;
	}
	return valid;
}
/// BCD形式の日付を変換
void ConvYYMMDDToTm(wxUint8 yy, wxUint8 mm, wxUint8 dd, struct tm *tm)
{
	tm->tm_year = (yy >> 4) * 10 + (yy & 0xf);
	if (0 <= tm->tm_year && tm->tm_year < 80) tm->tm_year += 100;
	tm->tm_mon = (mm >> 4) * 10 + (mm & 0xf);
	tm->tm_mon--;
	tm->tm_mday = (dd >> 4) * 10 + (dd & 0xf);
}
/// BCD形式の日付に変換
void ConvTmToYYMMDD(const struct tm *tm, wxUint8 &yy, wxUint8 &mm, wxUint8 &dd)
{
	yy = (wxUint8)(((tm->tm_year / 10) % 10) << 4) + (tm->tm_year % 10);
	mm = (wxUint8)(((tm->tm_mon + 1) / 10) << 4) + ((tm->tm_mon + 1) % 10);
	dd = (wxUint8)((tm->tm_mday / 10) << 4) + (tm->tm_mday % 10);
}
/// 日付を文字列で返す
wxString FormatYMDStr(const struct tm *tm)
{
	if (tm->tm_year >= 0 && tm->tm_mon >= -1) {
		return wxString::Format(wxT("%04d/%02d/%02d"), tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
	} else {
		return wxT("----/--/--");
	}
}
/// 時分秒を文字列で返す
wxString FormatHMSStr(const struct tm *tm)
{
	if (tm->tm_hour >= 0 && tm->tm_min >= 0) {
		return wxString::Format(wxT("%02d:%02d:%02d"), tm->tm_hour, tm->tm_min, tm->tm_sec);
	} else {
		return wxT("--:--:--");
	}
}
/// 時分を文字列で返す
wxString FormatHMStr(const struct tm *tm)
{
	if (tm->tm_hour >= 0 && tm->tm_min >= 0) {
		return wxString::Format(wxT("%02d:%02d"), tm->tm_hour, tm->tm_min);
	} else {
		return wxT("--:--");
	}
}

int ToInt(const wxString &val)
{
	long lval = 0;
	wxString h = val.Left(2).Lower();
	if (h == wxT("0x")) {
		val.Mid(2).ToLong(&lval, 16);
	} else if (h == wxT("0b")) {
		val.Mid(2).ToLong(&lval, 2);
	} else {
		val.ToLong(&lval);
	}
	return (int)lval;
}

bool ToBool(const wxString &val)
{
	bool bval = false;
	if (val == wxT("1") || val.Upper() == wxT("TRUE")) {
		bval = true;
	}
	return bval;
}

/// エスケープ文字を展開
/// @note \\\\                \\ そのもの
/// @note \\x[0-9a-f][0-9a-f] 16進数で指定
wxString Escape(const wxString &src)
{
	wxString str = src;
	wxString rstr;

	while (str.Length() > 0) {
		if (str.Left(2) == wxT("\\\\")) {
			rstr += wxT("\\");
			str = str.Mid(2);
		} else if (str.Left(2) == wxT("\\x")) {
			long v;
			str.Mid(2,2).ToLong(&v, 16);
			rstr += wxString((char)v, 1);
			str = str.Mid(4);
		} else {
			rstr += str.Left(1);
			str = str.Mid(1);
		}
	}
	return rstr;
}

wxString GetSideNumStr(int side_number, bool each_sides)
{
	wxString str;
	if (side_number >= 0) {
		if (each_sides) {
			str = wxString::Format(wxT("%d"), side_number);
		} else {
			str = wxString::Format(wxT("%c"), side_number + 0x41);
		}
	}
	return str;
}

wxString GetSideStr(int side_number, bool each_sides)
{
	wxString str;
	if (side_number >= 0) {
		if (each_sides) {
			str = wxString::Format(_("side %d"), side_number);
		} else {
			str = wxString::Format(_("side %c"), side_number + 0x41);
		}
	}
	return str;
}

}; /* namespace L3DiskUtils */

