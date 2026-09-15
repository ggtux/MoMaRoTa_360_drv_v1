#pragma once
inline unsigned long esp_random() { static unsigned long n = 10; return ++n; }
