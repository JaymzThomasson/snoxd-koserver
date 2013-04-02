#pragma once

#include <ctime>

class DateTime
{
public:
	// Uses the current time by default
	DateTime()
	{
		time_t now;
		time(&now);
		_tm = localtime(&now);
	}

	// Uses the timestamp specified
	DateTime(time_t timestamp)
	{
		_tm = localtime(&timestamp);
	}

	// Constructs a date/time using the specified date parts.
	DateTime(uint16 sYear, uint8 bMonth, uint8 bDay, uint8 bHour = 0, uint8 bMinute = 0, uint8 bSecond = 0)
	{
		// Get the current time
		time_t now;
		time(&now);
		_tm = localtime(&now);

		// Now update it with the data specified
		_tm->tm_year = sYear - 1900;
		_tm->tm_mon = bMonth - 1;
		_tm->tm_mday = bDay;
		_tm->tm_hour = bHour;
		_tm->tm_min = bMinute;
		_tm->tm_sec = bSecond;

		// Finally reconstruct it, so the other data is updated.
		Update();
	}

	// Uses the specified time struct
	DateTime(struct tm * _tm)
	{
		this->_tm = _tm;
	}

	// Simple getters to retrieve & convert time data to a more conventional form
	uint16 GetYear() { return _tm->tm_year + 1900; }
	uint8 GetMonth() { return _tm->tm_mon + 1; }
	uint8 GetDay() { return _tm->tm_mday; }
	uint8 GetDayOfWeek() { return _tm->tm_wday; }
	uint8 GetHour() { return _tm->tm_hour; }
	uint8 GetMinute() { return _tm->tm_min; }
	uint8 GetSecond() { return _tm->tm_sec; }

	// NOTE: If any of these overflow, they'll be handled by mktime() accordingly.
	// This makes our life *much* easier; date/time logic is not pretty.

	void __forceinline AddYears(int iYears)
	{
		_tm->tm_year += iYears;
		Update();
	}

	void __forceinline AddMonths(int iMonths)
	{
		_tm->tm_mon += iMonths;
		Update();
	}

	void __forceinline AddWeeks(int iWeeks)
	{
		AddDays(iWeeks * 7);
	}

	void __forceinline AddDays(int iDays)
	{
		_tm->tm_mday += iDays;
		Update();
	}

	void __forceinline AddHours(int iHours)
	{
		_tm->tm_hour += iHours;
		Update();
	}

	void __forceinline AddMinutes(int iMinutes)
	{
		_tm->tm_min += iMinutes;
		Update();
	}

	void __forceinline AddSeconds(int iSeconds)
	{
		_tm->tm_sec += iSeconds;
		Update();
	}

private:
	void __forceinline Update() { mktime(_tm); }

protected:
	struct tm * _tm;
};