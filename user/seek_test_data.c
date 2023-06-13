#ifndef TESTS_H
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#endif

int test_seek(int print) {
  int fd = open("testfile.txt", O_RDWR | O_CREATE | O_TRUNC);
  if (fd < 0) {
    printf("error: open test file on seek test\n");
    return 0;
  }

  char expected1[] = "Hello, World!\n";
  char expected2[] = "Hello, World!\n\0This is new data!\n";
  char expected3[] = "ta!\n";
  char expected4[] = "Hello, World!\n\0This is new data!\n\0Extra fresh data!\n";

  // Write some data to the beginning of the file
  seek(fd, 0, SEEK_SET);
  char write1[] = "Hello, World!\n";
  write(fd, write1, sizeof(write1));

  // Seek to the beginning of the file to read and print the written data
  seek(fd, 0, SEEK_SET);
  char actual1[20];
  read(fd, actual1, sizeof(actual1));
  if (memcmp(expected1, actual1, strlen(expected1)) == 0)
  {
    if (print) 
      printf("Writing and reading \"Hello, World!\\n\" successful!\n\n");
  }
  else 
  {
    printf("ERROR: FAILURE at writing or reading \"Hello, World!\\n\"\n\n");
    close(fd);
    return 0;
  }

  if (print) printf("File content after first write:\n");
  for(int i = 0; i < sizeof(actual1); i++)
    if (print) printf("%c", actual1[i]);
  if (print) printf("\n");

  // Seek to the end of the file to write additional data
  seek(fd, 0, SEEK_CUR);
  char write2[] = "This is new data!\n";
  write(fd, write2, sizeof(write2));

  // Testing serek with negative offset whose absolute value is larger than filesize
  // should seek to offset 0
  seek(fd, -50, SEEK_CUR);
  
  // Read the updated data from the beginning of the file
  char actual2[40];
  read(fd, actual2, sizeof(actual2));

  if (memcmp(expected2, actual2, sizeof(expected2)) == 0)
  {
    if (print) printf("Concatenating \"This is new data!\\n\" successful!\n\n");
  }
  else
  {
    printf("ERROR: FAILURE to concatenate or read entire data from the file\n\n");
    close(fd);
    return 0;
  }
  if (print) printf("File content after second write:\n");
  for(int i = 0; i < sizeof(actual2); i++)
    if (print) printf("%c", actual2[i]);
  if (print) printf("\n");

  // Seek slightly backwards then re-read
  seek(fd, -5, SEEK_CUR);
  char actual3[40];
  read(fd, actual3, sizeof(actual3));
  if (memcmp(expected3, actual3, sizeof(expected3)) == 0)
  {
    if (print) printf("Moving backwards 5 bytes then reading is successful!\n\n");
  }
  else 
  {
    printf("ERROR: FAILURE to go back 5 bytes and read content properly\n\n");
    close(fd);
    return 0;
  }
  if (print) printf("Read after going slightly backwards:\n%s\n", actual3);

  // Go too far to write new stuff
  seek(fd, 500, SEEK_SET);
  char write3[] = "Extra fresh data!\n";
  write(fd, write3, sizeof(write3));

  seek(fd, 0, SEEK_SET);
  char actual4[70];
  read(fd, actual4, sizeof(actual4));

  if (memcmp(expected4, actual4, sizeof(expected4)) == 0)
  {
    if (print) printf("Concatenating \"Extra fresh data!\\n\" successful!\n\n");
  }
  else 
  {
    printf("ERROR: FAILURE to concatenate or read extra data\n\n");
    close(fd);
    return 0;
  }

  if (print) printf("File content after third write:\n");
  for(int i = 0; i < sizeof(actual4); i++)
    if (print) printf("%c", actual4[i]);
  if (print) printf("\n");

  close(fd);
  return 1;
}

uint8 lfsr_char(uint8 lfsr)
{
  uint8 bit;
  bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 4)) & 0x01;
  lfsr = (lfsr >> 1) | (bit << 7);
  return lfsr;   
}

void reset_seed(){
  int fd = open("random", O_RDWR);
  if (fd < 0) {
    printf("ERROR ON openning 'random' file\n");
  }
  uint8 originalSeed = 0x2A;
  int writeResult = write(fd, &originalSeed, 1);
  if (writeResult == -1)
  {
      printf("ERROR ON RESETING TO ORIGINAL SEED\n");
  }
  else printf("RESET SEED TO THE ORIGINAL SEED!\n");
}

int test_random_m(int print){
  int fd = open("random", O_RDWR);
  if (fd < 0) {
    printf("ERROR ON openning 'random' file\n");
    return 0;
  }
  
  int expected[255];
  expected[0] = lfsr_char(0x2A);
  for(int i = 1; i < 255; i++)
  {
    expected[i] = lfsr_char(expected[i-1]);
  }

  uint8 res;
  for(int i = 1; i < 255; i++)
  {
    read(fd, &res, 1);
    if (print) printf("test random expected: %d\n", expected[i]);
  }

  printf("FINISHED random test m\n");
  return 1;
}

int test_random(int reset, int print) {
  int fd = open("random", O_RDWR);
  if (fd < 0) {
  printf("error: open\n");
  return 0;
  }
  
  // IMPORTANT NOTE: RE-RUNNING THIS TEST SHOULD FAIL! 
  // BY WRITING A NEW SEED AT THE END OF THE TEST, 
  // THE DRIVER SHOULD HAVE A NEW SEED AT THE END, 
  // RUN TESTS WITH RESET > 0 TO RESET TO THE ORIGINAL SEED EVERY RUN
  if (reset)
  {
      uint8 originalSeed = 0x2A;
      int writeResult = write(fd, &originalSeed, 1);
      if (writeResult == -1)
      {
          printf("ERROR ON RESETING TO ORIGINAL SEED\n");
          return 0;
      }
      else printf("RESET SEED TO THE ORIGINAL SEED!\n");
  }

  int expected[255];
  expected[0] = lfsr_char(0x2A);
  for(int i = 1; i < 255; i++)
  {
    expected[i] = lfsr_char(expected[i-1]);
  }

  uint8 res;
  read(fd, &res, 1);
  if (res == expected[0])
  {   
      if (print) printf("Success on generating first pseudo-random number from original seed\n");
  }
  else
    {
      printf("ERORR on generating first pseudo-random number from original seed\n");
      return 0;
    }
  // Make sure there are no errors in generating following prng values
  for(int i = 1; i < 255; i++)
  {
      int ret = read(fd, &res, 1);
      if (ret != 1 || res != expected[i])
      {
        printf("ERROR on generating new pseudo-random number from previous number\n");
        printf("result: %d, expected: %d\n", res, expected[i]);
        close(fd);
        return 0;
      }
  }
    
  if (print) printf("Successfully generated all 255 variants of the original seed!\n");
  read(fd, &res, 1);
  if (res == expected[0])
    {
      if (print) printf("Successfully looped through all values and returned to initial value!\n");
    }
  else
  {
    printf("ERROR on looping through all values and returning to initial value\n");
    close(fd);
    return 0;
  }

  // Test changing the seed and getting new stuff:
  uint8 val = 0x4C;
  int writeResult = write(fd, (void*)&val, 6);
  if (writeResult != -1)
  {
    printf("ERROR on writing new seed with n != 1\n");
    close(fd);
    return 0;
  }
  writeResult = write(fd, (void*)&val, 1);
  if (writeResult == -1)
  {
    printf("ERROR on writing new seed which should've succeeded\n");
    close(fd);
    return 0;
  }
  if (print) printf("Writing new seed 0x4C successfull\n");
  
  int lfsrOfNewSeed = lfsr_char(0x4C);
  read(fd, &res, 1);
  if (res != lfsrOfNewSeed)
  {
    printf("ERROR on generating new PRNG\n");
    close(fd);
    return 0;
  }
  printf("SUCCESS!! PASSED RANDOM TESTS!\n");
  close(fd);
  return 1;
}
