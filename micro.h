#ifndef MICRO_H_
#define MICRO_H_

#define CTRL_KEY(k) ((k)&0x1f)

void die(const char *s);
void disableRawInputMode();
void enableRawInputMode();
char microReadKey();
void microProcessKeypress();

#endif // MICRO_H_