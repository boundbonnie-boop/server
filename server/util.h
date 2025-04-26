#pragma once
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <cstdint>

namespace server {
pid_t GetThreadId();
uint32_t GetFiberId();

void Backtrace(std::vector<std::string>& bt, int size = 64, int skip = 1);
std::string BacktraceToString(int size = 64, int skip = 2, const std::string& prefix = "");

//时间ms
uint64_t GetCurrentMS();
uint64_t GetCurrentUS();

}