
#ifndef __MJSAVE_H__
#define __MJSAVE_H__

#include <Arduino.h>
#include <FS.h>
//#include <SD.h>
#include <SPI.h>

/*
void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
void createDir(fs::FS &fs, const char * path);
void removeDir(fs::FS &fs, const char * path);
void readFile(fs::FS &fs, const char * path);
void writeFile(fs::FS &fs, const char * path, const char * message);
void appendFile(fs::FS &fs, const char * path, const char * message);
void renameFile(fs::FS &fs, const char * path1, const char * path2);
void deleteFile(fs::FS &fs, const char * path);
void testFileIO(fs::FS &fs, const char * path);
*/

void listDir(FS &fs, String dirname, uint8_t levels);
//void listDir(FS &fs, const char * dirname, uint8_t levels);
void createDir(FS &fs, const char * path);
void removeDir(FS &fs, const char * path);
uint32_t readFile(FS &fs, const char * path);
void writeFile(FS &fs, const char * path, const char * message);
void appendFile(FS &fs, const char * path, const char * message);
void renameFile(FS &fs, const char * path1, const char * path2);
void deleteFile(FS &fs, const char * path);
//void testFileIO(FS &fs, const char * path);

void readLine(FS &fs, int n);
//void readLine(FS &fs, const char * path);


#endif //__MJSAVE_H__
