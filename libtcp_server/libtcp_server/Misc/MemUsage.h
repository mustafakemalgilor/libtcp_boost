#pragma once

class MemUsage
{
public:
	size_t getPeakRSS();
	size_t getCurrentRSS();
	MemUsage();
	~MemUsage();
};

