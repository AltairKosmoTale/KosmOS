#pragma once

enum LogLevel {
  kError = 3,
  kWarn  = 4,
  kInfo  = 6,
  kDebug = 7,
};

/* 글로벌 로그 우선순위 임계값 level로 설정,
이후의 Log 호출에서는 설정된 level 이상의 로그만 기록 */
void SetLogLevel(LogLevel level);

/* 지정된 우선 순위가 임계값 이상이면 기록, 미만이면 폐기 */
int Log(LogLevel level, const char* format, ...);
