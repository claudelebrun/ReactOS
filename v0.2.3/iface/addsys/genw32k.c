/* $Id: genw32k.c,v 1.10 2004/04/12 22:07:45 hyperion Exp $
 *
 * COPYRIGHT:             See COPYING in the top level directory
 * PROJECT:               ReactOS version of ntdll
 * FILE:                  iface/native/genntdll.c
 * PURPOSE:               Generates the system call stubs in ntdll
 * CHANGE HISTORY:	  Added a '@xx' to deal with stdcall [ Ariadne ]
 * 	19990616 (ea)
 * 		Four arguments now required; 4th is the file
 * 		for ntoskrnl ZwXXX functions (which are merely calls
 * 		to twin NtXXX calls, via int 0x2e (x86).
 * 	19990617 (ea)
 * 		Fixed a bug in function numbers in kernel ZwXXX stubs.
 *
 */

/* INCLUDE ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PARAMETERIZED_LIBS

#define INPUT_BUFFER_SIZE 255

#define INDEX  0x1000		/* SSDT index 1 */


/* FUNCTIONS ****************************************************************/

void write_stub_header(FILE * out)
{
 fputs
 (
  "/* Machine generated, don't edit */\n"
  "\n"
  "#ifdef __cplusplus\n"
  "#define EXTERN_C extern \"C\"\n"
  "#else\n"
  "#define EXTERN_C\n"
  "#endif\n"
  "\n"
  "EXTERN_C static __inline__ __attribute__((regparm(2)))"
  "void*ZwRosSystemServiceThunk(long n,void*a)"
  "{"
   "void*ret;"
   "__asm__"
   "("
    "\"int $0x2E\":"
    "\"=a\"(ret):"
    "\"a\"(n),\"d\"(a)"
   ");"
   "return ret;"
  "}\n",
  out
 );
}

void write_syscall_stub_func(FILE* out, char* name, unsigned nr_args,
                             unsigned int sys_call_idx)
{
 unsigned i;

 fprintf(out, "EXTERN_C void*__stdcall %s(", name);
 
 if(nr_args == 0)
  fputs("void", out);
 else
  for(i = 0; i < nr_args; ++ i)
  {
   if(i > 0)
    fputs(",", out);

   fprintf(out, "void*a%u", i);
  }
 
 fputs("){", out);

 if(nr_args > 1)
  for(i = 1; i < nr_args; ++ i)
   fprintf(out, "(void)a%u;", i);

 fprintf(out, "return ZwRosSystemServiceThunk(%u,", sys_call_idx);
 
 if(nr_args == 0)
  fputs("0", out);
 else
  fputs("&a0", out);
 
 fputs(");}\n", out);
}

void write_syscall_stub(FILE* out1, FILE* out2, char* name,
			unsigned nr_args, unsigned int sys_call_idx)
{
  write_syscall_stub_func(out1, name, nr_args, sys_call_idx);
  write_syscall_stub_func(out2, name, nr_args, sys_call_idx);
}

int makeSystemServiceTable(FILE *in, FILE *out)
{
char    line [INPUT_BUFFER_SIZE];
char    *s;
char    *name;
int     sys_call_idx;
char    *snr_args;

	/*
	 * Main SSDT Header
	 */
	fprintf(out,"// Machine generated, don't edit\n");
	fprintf(out,"\n\n");

	/*
	 * First we build the Win32k SSDT
	 */
	fprintf(out,"SSDT Win32kSSDT[] = {\n");

	/* First system call has index zero */
	sys_call_idx = 0;

	/* Go on until EOF or read zero bytes */
	while ((!feof(in))&& (fgets(line, sizeof line, in) != NULL))
	{
		if ((s = (char *) strchr(line,'\r')) != NULL)
		{
			*s = '\0';
		}
		/*
		 * Skip comments (#) and empty lines.
		 */
		s = & line[0];
		if ((*s) != '#' && (*s) != '\0')
		{
			/* Extract the NtXXX name */
			name = (char *)strtok(s," \t");

#ifdef VERBOSE
			printf("%3d \"%s\"\n",sys_call_idx | INDEX,name);
#endif

			if (sys_call_idx > 0)
			{
				fprintf(out,",\n");
			}
			/*
			 * Now write the current system call's name
			 * in the service table.
			 */
			fprintf(out,"\t\t(PVOID (NTAPI *)(VOID))%s",name);

			/* Next system call index */
			sys_call_idx++;
		}
	}
	/* Close the service table (C syntax) */
	fprintf(out,"\n};\n");

	/*
	 * Now we build the Win32k SSPT
	 */
	rewind(in);
	fprintf(out,"\n\n");
	fprintf(out,"SSPT Win32kSSPT[] = {\n");

	/* First system call has index zero */
	sys_call_idx = 0;

	/* Go on until EOF or read zero bytes */
	while ((!feof(in))&& (fgets(line, sizeof line, in) != NULL))
	{
		if ((s = (char *) strchr(line,'\r')) != NULL)
		{
			*s = '\0';
		}
		/*
		 * Skip comments (#) and empty lines.
		 */
		s = & line[0];
		if ((*s) != '#' && (*s) != '\0')
		{
			/* Extract the NtXXX name */
			name = (char *)strtok(s," \t");
			/* Extract the stack size */
			snr_args = (char *)strtok(NULL," \t");

#ifdef VERBOSE
			printf("%3d \"%s\"\n",sys_call_idx|INDEX,name);
#endif

			if (sys_call_idx > 0)
			{
				fprintf(out,",\n");
			}
			/*
			 * Now write the current system call's ID
			 * in the service table along with its Parameters Size.
			 */
			fprintf(out,"\t\t%lu * sizeof(void*)", strtoul(snr_args, NULL, 0));

			/* Next system call index */
			sys_call_idx++;
		}
	}
	/*
	 * Close the service table (C syntax)
	 */
	fprintf(out,"\n};\n");

	/*
	 * We write some useful defines
	 */
	fprintf(out, "\n\n#define MIN_SYSCALL_NUMBER    0\n");
	fprintf(out, "#define MAX_SYSCALL_NUMBER    %d\n", sys_call_idx-1);
	fprintf(out, "#define NUMBER_OF_SYSCALLS    %d\n", sys_call_idx);
	fprintf(out, "ULONG Win32kNumberOfSysCalls = %d;\n", sys_call_idx);

	return(0);
}


int
process(
	FILE	* in,
	FILE	* out1,
	FILE	* out2
	)
{
	char		line [INPUT_BUFFER_SIZE];
	char		* s;
	char		* name;		/* NtXXX name */
	int		sys_call_idx;	/* NtXXX index number in the service table */
	char		* snr_args;	/* stack_size / machine_word_size */

	/*
	 * GDI32 stubs file header
	 */
	write_stub_header(out1);

	/*
	 * USER32 stubs file header
	 */
	write_stub_header(out2);

	/*
	 * Scan the database. DB is a text file; each line
	 * is a record, which contains data for one system
	 * function. Each record has three columns:
	 *
	 * NT_NAME	(e.g. NtCreateProcess)
	 * ZW_NAME	(e.g. ZwCreateProcess)
	 * STACK_SIZE	(in machine words: for x[3456]86
	 * 		processors a machine word is 4 bytes)
	 */

	/* First system call has index zero */
	sys_call_idx = 0;
	while (
		/* Go on until EOF or read zero bytes */
		(	(!feof(in))
			&& (fgets(line, sizeof line, in) != NULL)
			)
		)
	{
		/*
		 * Remove, if present, the trailing CR.
		 * (os specific?)
		 */
		if ((s = (char *) strchr(line,'\r')) != NULL)
		{
			*s = '\0';
		}
		/*
		 * Skip comments (#) and empty lines.
		 */
		s = & line[0];
		if ((*s) != '#' && (*s) != '\0')
		{
			/* Extract the NtXXX name */
			name = (char *)strtok(s," \t");
			/* Extract the stack size */
			snr_args = (char *)strtok(NULL," \t");

#ifdef VERBOSE
			printf("%3d \"%s\"\n",sys_call_idx | INDEX,name);
#endif

                        write_syscall_stub(out1, out2, name, strtoul(snr_args, NULL, 0), sys_call_idx | INDEX);

			/* Next system call index */
			sys_call_idx++;
		}
	}

	return(0);
}

void usage(char * argv0)
{
	printf("Usage: %s w32ksvc.db w32k.lst ssdt.h win32k.c win32k.c\n"
	       "  w32ksvc.db    input file(system calls database)\n"
	       "  w32k.lst      system functions database\n"
	       "  ssdt.h        WIN32K service table\n"
	       "  win32k.c      GDI32 stubs\n"
	       "  win32k.c      USER32 stubs\n",
		argv0
		);
}

int main(int argc, char* argv[])
{
	FILE	* in;	/* System calls database */
	FILE	* out1;	/* SERVICE_TABLE */
	FILE	* out2;	/* GDI32 stubs */
	FILE	* out3;	/* USER32 stubs */
	int	ret;

	if (argc != 5)
	{
		usage(argv[0]);
		return(1);
	}

	in = fopen(argv[1],"rb");
	if (in == NULL)
	{
		perror("Failed to open input file (system calls database)");
		return(1);
	}

	out1 = fopen(argv[2],"wb");
	if (out1 == NULL)
	{
		perror("Failed to open output file (WIN32K service table)");
		return(1);
	}

	out2 = fopen(argv[3],"wb");
	if (out2 == NULL)
	{
		perror("Failed to open output file (GDI32 stubs)");
		return(1);
	}

	out3 = fopen(argv[4],"wb");
	if (out3 == NULL)
	{
		perror("Failed to open output file (USER32 stubs)");
		return(1);
	}

	ret = process(in,out2,out3);
	rewind(in);
	ret = makeSystemServiceTable(in, out1);

	fclose(in);
	fclose(out1);
	fclose(out2);
	fclose(out3);

	return(ret);
}
