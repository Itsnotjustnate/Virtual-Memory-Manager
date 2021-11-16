//
//  memmgr.c
//  memmgr
//
//  Created by William McCarthy on 17/11/20.
//  Copyright © 2020 William McCarthy. All rights reserved.
//
//
//  Initial code from William McCarthy
/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#define ARGC_ERROR 1
#define FILE_ERROR 2
#define BUFLEN 256
#define FRAME_SIZE  256


//-------------------------------------------------------------------
unsigned getpage(unsigned x) { return (0xff00 & x) >> 8; }

unsigned getoffset(unsigned x) { return (0xff & x); }

void getpage_offset(unsigned x) {
  unsigned  page   = getpage(x);
  unsigned  offset = getoffset(x);
  printf("x is: %u, page: %u, offset: %u, address: %u, paddress: %u\n", x, page, offset,
         (page << 8) | getoffset(x), page * 256 + offset);
}

typedef struct {
  bool present, used;
} PT_entry;

typedef struct 
{
  unsigned frame_num;
  unsigned page_num;
  PT_entry entry[PT_SIZE];
} TLB;


int check_TLB(PT_entry (*TLB)[TLB_SIZE], unsigned page) {
  for (int i = 0; i < TLB_SIZE; i++) {
    if ((*TLB)[i].page_num == page) {
      return i;
    }
  }
}

void TLB_add(PT_entry (*TLB)[TLB_SIZE], int index, PT_entry entry) {
  (*TLB)[index] = entry;
}

void TLB_remove(PT_entry (*TLB)[TLB_SIZE], int index) {
  (*TLB)[index].page_num = (unsigned int)-1;
  (*TLB)[index].present = false;
}

int main(int argc, const char* argv[]) {
  FILE* fadd = fopen("addresses.txt", "r");    // open file addresses.txt  (contains the logical addresses)
  if (fadd == NULL) { fprintf(stderr, "Could not open file: 'addresses.txt'\n");  exit(FILE_ERROR);  }

  FILE* fcorr = fopen("correct.txt", "r");     // contains the logical and physical address, and its value
  if (fcorr == NULL) { fprintf(stderr, "Could not open file: 'correct.txt'\n");  exit(FILE_ERROR);  }

  FILE* fback = fopen("BACKING_STORE.bin", "rb");    //opens file BACKING_STORE.bin
  if (fback == NULL) {
    fprintf(stderr, "Could not open file: 'BACKING_STORE.bin'\n");
    exit(FILE_ERROR);
  }


  char buf[BUFLEN];
  unsigned   page, offset, physical_add, frame = 0;
  unsigned   logic_add;                  // read from file address.txt
  unsigned   virt_add, phys_add, value;  // read from file correct.txt
  unsigned   val;

  unsigned int prev_frame = 0;



  printf("ONLY READ FIRST 20 entries -- TODO: change to read all entries\n\n");

  // not quite correct -- should search page table before creating a new entry
      //   e.g., address # 25 from addresses.txt will fail the assertion
      // TODO:  add page table code
      // TODO:  add TLB code
   int frames_used = 0;

  bool mem_full = false;

  int p_fault = 0;
  int tlb_hit = 0;

  bool replace_policy = 0;

  char* frame_ptr;

  frame_ptr = (char*)malloc(FRAME_NUM * FRAME_SIZE);

  PT_entry PT[PT_SIZE];

  for (int i = 0; i < PT_SIZE; i++) {
    PT[i].page_num = (unsigned int)i;
    PT[i].present = false;
    PT[i].used = false;
  }

  int tlb_track = 0;

  for (int o = 0; o < 1000; o++) {

    fscanf(fcorr, "%s %s %d %s %s %d %s %d", buf, buf, &virt_add,
           buf, buf, &phys_add, buf, &value);  // read from file correct.txt

    fscanf(fadd, "%d", &logic_add);  // read from file address.txt
    page   = getpage(  logic_add);
    offset = getoffset(logic_add);

    int result = check_TLB(&TLB, page);
    if (result >= 0) {
      tlb_hit++;

      frame = TLB[result].frame_num;

      page = TLB[result].page_num;

      PT[page].used = true;
    }
    else if (PT[page].present) {
      frame = PT[page].frame_num;

      PT[page].used = true;

      TLB_add(&TLB, tlb_track++, PT[page]);

      if (tlb_track > 15)
        tlb_track = 0;
    }
  else {
    p_fault++;

    if (frames_used >= FRAME_NUM) {
      if (replace_policy == 0)
        frames_used = 0;
      mem_full = true;
    }
    frame = frames_used;
    
    if (mem_full) {
      if (replace_policy == 0) {
        int p = find_frame_PT(&PT, frame);
        if (p != -1) {
          PT[p].present = false;
        }
      }
      else {
        unsigned int p = get_used_PT(&PT);

        frame = PT[p].frame_num;

        PT[p].present = false;

        int t = check_TLB(&TLB, (unsigned int)p);
        if(t != -1)
          TLB_remove(&TLB, t);
      }
    }

    fseek(fback, page * FRAME_SIZE, SEEK_SET);

    fread(buf, FRAME_SIZE, 1, fback);

    for (int i = 0; i < FRAME_SIZE; i++) {
      *(frame_ptr + (frame * FRAME_SIZE) + i) = buf[i];
    }

    update_frame_PT(&PT, page, frame);

    TLB_add(&TLB, tlb_track++, PT[page]);

    if (tlb_track > 15)
      tlb_track = 0;
    
    frames_used++;
  }

    physical_add = frame++ * FRAME_SIZE + offset;
    
    assert(physical_add == phys_add);

    val = *(frame_ptr + physical_add);

    assert(val == value);
    
    // todo: read BINARY_STORE and confirm value matches read value from correct.txt
    
    printf("logical: %5u (page: %3u, offset: %3u) ---> physical: %5u (frame: %3u)---> value: %4d -- passed\n", logic_add, page, offset, physical_add, frame, (signed int)val);
    if (frame < prev_frame) {     //check page table
       printf("   HIT!\n");
      }
    else { 
      prev_frame = frame;
      printf("\n");
    }
    if (o % 5 == 0) { printf("\n"); }
  }

  printf("\nPage Fault Percentage: %1.3f%%", (double)p_fault / 1000);
  printf("\nTLB Hit Percentage: %1.3f%%\n\n", (double)tlb_hit / 1000);
  printf("ALL logical ---> physical assertions PASSED!\n");
  printf("\n\t\t...done.\n");

  fclose(fback);
  fclose(fcorr);
  fclose(fadd);
  
  printf("ONLY READ FIRST 20 entries -- TODO: change to read all entries\n\n");
  
  printf("ALL logical ---> physical assertions PASSED!\n");
  printf("!!! This doesn't work passed entry 24 in correct.txt, because of a duplicate page table entry\n");
  printf("--- you have to implement the PTE and TLB part of this code\n");

//  printf("NOT CORRECT -- ONLY READ FIRST 20 ENTRIES... TODO: MAKE IT READ ALL ENTRIES\n");

  printf("\n\t\t...done.\n");

  //deallocating memory
  free(frame_ptr);
  frame_ptr = NULL;

  return 0;
}

*/

//  
//  memmgr.c
//
//  memmgr
//  Created by Nathan Eduvala
//
//  Copyright © 2020 William McCarthy. All rights reserved.
//
//  ===       Start of Code       ===

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#define FILE_ERROR 2

int main() {
  printf("Hello world!");
  return 0;
}