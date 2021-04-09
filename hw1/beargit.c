#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>

#include "beargit.h"
#include "util.h"

/* Implementation Notes:
 *
 * - Functions return 0 if successful, 1 if there is an error.
 * - All error conditions in the function description need to be implemented
 *   and written to stderr. We catch some additional errors for you in main.c.
 * - Output to stdout needs to be exactly as specified in the function description.
 * - Only edit this file (beargit.c)
 * - You are given the following helper functions:
 *   * fs_mkdir(dirname): create directory <dirname>
 *   * fs_rm(filename): delete file <filename>
 *   * fs_mv(src,dst): move file <src> to <dst>, overwriting <dst> if it exists
 *   * fs_cp(src,dst): copy file <src> to <dst>, overwriting <dst> if it exists
 *   * write_string_to_file(filename,str): write <str> to filename (overwriting contents)
 *   * read_string_from_file(filename,str,size): read a string of at most <size> (incl.
 *     NULL character) from file <filename> and store it into <str>. Note that <str>
 *     needs to be large enough to hold that string.
 *  - You NEED to test your code. The autograder we provide does not contain the
 *    full set of tests that we will run on your code. See "Step 5" in the homework spec.
 */

/* beargit init
 *
 * - Create .beargit directory
 * - Create empty .beargit/.index file
 * - Create .beargit/.prev file containing 0..0 commit id
 *
 * Output (to stdout):
 * - None if successful
 */

int beargit_init(void) {
  fs_mkdir(".beargit");

  FILE* findex = fopen(".beargit/.index", "w");
  fclose(findex);
  
  write_string_to_file(".beargit/.prev", "0000000000000000000000000000000000000000");

  return 0;
}


/* beargit add <filename>
 * 
 * - Append filename to list in .beargit/.index if it isn't in there yet
 *
 * Possible errors (to stderr):
 * >> ERROR: File <filename> already added
 *
 * Output (to stdout):
 * - None if successful
 */

int beargit_add(const char* filename) {
  //open pointers to files
  FILE* findex = fopen(".beargit/.index", "r");
  FILE *fnewindex = fopen(".beargit/.newindex", "w");

  char line[FILENAME_SIZE];
  while(fgets(line, sizeof(line), findex)) {
    //strtok will tokenize filename and break them up into tokens
    //with newline as delimiter. We do this as a way to remove the newline
    //'line' is mutated to strip newline.
    strtok(line, "\n");
    if (strcmp(line, filename) == 0) {
      fprintf(stderr, "ERROR: File %s already added\n", filename);
      fclose(findex);
      fclose(fnewindex);
      fs_rm(".beargit/.newindex");
      return 3;
    }

    fprintf(fnewindex, "%s\n", line);
  }

  fprintf(fnewindex, "%s\n", filename);
  fclose(findex);
  fclose(fnewindex);

  fs_mv(".beargit/.newindex", ".beargit/.index");

  return 0;
}


/* beargit rm <filename>
 * 
 * See "Step 2" in the homework 1 spec.
 *
 */

int beargit_rm(const char* filename) {
  //open pointers to files
  FILE* findex = fopen(".beargit/.index", "r");
  FILE *fnewindex = fopen(".beargit/.newindex", "w");
  int removed = 0;

  char line[FILENAME_SIZE];
  while(fgets(line, sizeof(line), findex)) {
    //strtok will tokenize filename and break them up into tokens
    //with newline as delimiter. We do this as a way to remove the newline
    //'line' is mutated to strip newline.
    strtok(line, "\n");
    if (strcmp(line, filename) == 0) {
      removed = 1;
      continue;
    }
    fprintf(fnewindex, "%s\n", line);
  }
  if (!removed){
    fprintf(stderr, "ERROR: File %s is not tracked\n", filename);
    fclose(findex);
    fclose(fnewindex);
    fs_rm(".beargit/.newindex");
    return 1;

  }
  fclose(findex);
  fclose(fnewindex);
  fs_mv(".beargit/.newindex", ".beargit/.index");
  return 0;
}

/* beargit commit -m <msg>
 *
 * See "Step 3" in the homework 1 spec.
 *
 */
//pointer to constant
const char* go_bears = "GO BEARS!";

int is_commit_msg_ok(const char* msg) {
  const char *start_go_bears = go_bears;

  while(*msg != '\0' && *go_bears != '\0'){
    if (*msg == *go_bears && *go_bears != '\0'){
      msg++;
      go_bears++;

    }else if (*msg != *go_bears){
      msg++;
      go_bears = start_go_bears;
    }
  }
  //if at null character, that means we found all needed characters
  if (*go_bears == '\0'){
    go_bears = start_go_bears;
    return 1;
  }
  go_bears = start_go_bears;
  return 0;
}

void next_commit_id(char* commit_id) {
    for(int i = COMMIT_ID_BYTES;i > 0; i--){
        if(commit_id[i] == '0'){
            commit_id[i] = '6';
            break;
        }else if(commit_id[i] == '6'){
            commit_id[i] = '1';
            break;
        }else if(commit_id[i] == '1'){
            commit_id[i] = 'C';
            break;
        }else if(commit_id[i] == 'C'){
            continue;
        }
    }
}

int beargit_commit(const char* msg) {
  if (!is_commit_msg_ok(msg)) {
    fprintf(stderr, "ERROR: Message must contain \"%s\"\n", go_bears);
    return 1;
  }

  char commit_id[COMMIT_ID_SIZE];
  //create new commit ID based on old ID
  read_string_from_file(".beargit/.prev", commit_id, COMMIT_ID_SIZE);
  next_commit_id(commit_id);

  //create new directory
  char new_directory[strlen(".beargit/")+COMMIT_ID_SIZE];
  snprintf(new_directory, sizeof(new_directory),".beargit/%s", commit_id);
  fs_mkdir(new_directory);

  //create new .index file inside new directory
  char new_index[strlen(new_directory) + strlen("/.index") + 1];
  snprintf(new_index, strlen(new_directory) + strlen("/.index") + 1,"%s/.index", new_directory);
  fs_cp(".beargit/.index", new_index);

  //create new .prev file inside new directory
  char new_prev[strlen(new_directory) + strlen("/.prev") + 1];
  snprintf(new_prev, strlen(new_directory) + strlen("/.prev") + 1,"%s/.prev", new_directory);
  fs_cp(".beargit/.prev", new_prev);

  //we need to copy all tracked files by opening index and iterating through the files
  FILE* findex = fopen(".beargit/.index", "r");

  char line[FILENAME_SIZE];
  while(fgets(line, sizeof(line), findex)) {
    strtok(line, "\n");
    char copied_file[9 + COMMIT_ID_SIZE + FILENAME_SIZE];
    snprintf(copied_file, 100,"%s/%s", new_directory, line);
    fs_cp(line, copied_file);
  }
  fclose(findex);


  //once old .prev file is copied into new directory, write the 
  //current ID to .prev
  write_string_to_file(".beargit/.prev", commit_id);
  char new_msg[14 + COMMIT_ID_SIZE];
  snprintf(new_msg, strlen(new_msg),"%s/.msg", new_directory);
  write_string_to_file(new_msg, msg);
  return 0;
} 
/* beargit status
 *
 * See "Step 1" in the homework 1 spec.
 *
 */

int beargit_status() {
  FILE* findex = fopen(".beargit/.index", "r");

  char line[FILENAME_SIZE];

  printf("Tracked Files:\n");
  while(fgets(line, sizeof(line), findex)) {
    fprintf(stdout, "%s", line);
  }

  fclose(findex);
  return 0;
}

/* beargit log
 *
 * See "Step 4" in the homework 1 spec.
 *
 */

int beargit_log() {
  //get id from .beargit/.prev
  char prev[COMMIT_ID_SIZE];
  read_string_from_file(".beargit/.prev", prev, COMMIT_ID_SIZE);

  //use that id to start the while loop
    //while .prev != a bunch of zeros
    while(prev[COMMIT_ID_SIZE - 2]  != '0'){
      char msg_file[14 + COMMIT_ID_SIZE];
      snprintf(msg_file, 14 + COMMIT_ID_SIZE, ".beargit/%s/.msg", prev);
      char msg[MSG_SIZE];
      read_string_from_file(msg_file, msg, MSG_SIZE);

      printf("commit %s\n", prev);
      printf("%s\n", msg);

      //read next prev file
      char next_prev[15 + COMMIT_ID_SIZE];
      snprintf(next_prev, 15 + COMMIT_ID_SIZE ,".beargit/%s/.prev", prev);
      read_string_from_file(next_prev, prev, COMMIT_ID_SIZE);
    }
  return 0;
}
