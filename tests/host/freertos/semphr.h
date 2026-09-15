#pragma once
using SemaphoreHandle_t = void*;
inline SemaphoreHandle_t xSemaphoreCreateRecursiveMutex() { return (void*)1; }
inline void xSemaphoreTakeRecursive(SemaphoreHandle_t, unsigned long) {}
inline void xSemaphoreGiveRecursive(SemaphoreHandle_t) {}
