#include <carmen/carmen.h>

static char *next_word(char *str) {

  char *mark = str;

  mark += strcspn(mark, " \t");
  mark += strspn(mark, " \t");

  return mark;
}

static void add_time(char *progname, char *filename, float dt) {

  FILE *fp;
  char token[100];
  char line[2000], *mark;
  int n, len;
  double timestamp;

  if (strcmp(carmen_file_extension(filename), ".rlog"))
    carmen_die("usage: %s logfile.rlog <nsec>", progname);

  if (!carmen_file_exists(filename))
    carmen_die("Could not find file %s\n", filename);  

  fp = fopen(filename, "r");

  fgets(line, 2000, fp);
  for (n = 1; !feof(fp); n++) {
    //printf("---> n = %d, line = %s\n", n, line); 
    if (sscanf(line, "%s", token) == 1) {
      if (!strcmp(token, "ROOM")) {
	mark = line + strcspn(line, "\"");
	mark++;
	len = strcspn(mark, "\"");
	mark += len;
	mark = next_word(mark);
	sscanf(mark, "%lf", &timestamp);
	*mark = '\0';
	printf("%s %.2f\n", line, timestamp + dt);
      }
      else if (!strcmp(token, "ACTIVITY")) {
	mark = line + strcspn(line, "\"");
	mark++;
	len = strcspn(mark, "\"");
	mark += len;
	mark = next_word(mark);
	sscanf(mark, "%lf", &timestamp);
	*mark = '\0';
	printf("%s %.2f\n", line, timestamp + dt);
      }
      else
	printf("%s", line);
    }

    fgets(line, 2000, fp);
  }

  fclose(fp);
}

int main(int argc, char *argv[]) {

  if (argc != 3)
    carmen_die("usage: %s logfile.rlog <nsec>", argv[0]);

  add_time(argv[0], argv[1], atof(argv[2]));

  return 0;
}
