/* ### cpcn-1 */
/************************************************/
/*						*/
/*		small-c:PC compiler		*/
/*						*/
/*		  by Ron Cain			*/
/*	modified by CAPROCK SYSTEMS for IBM PC	*/
/*						*/
/************************************************/


#define BANNER  "* * *  Small-C:PC  V1.1  * * *"


#define VERSION "PC-DOS Version N: June, 1982"


#define AUTHOR "By Ron Cain, Modified by CAPROCK SYSTEMS for IBM PC"


/*	Define system dependent parameters	*/


/*	Stand-alone definitions			*/


#define NULL 0
#define eol 13


/*	UNIX definitions (if not stand-alone)	*/


/* #include <stdio.h>	*/
/* #define eol 10	*/


/*	Define the symbol table parameters	*/


#define	symsiz	14
#define	symtbsz	5040
#define numglbs 300
#define	startglb symtab
#define	endglb	startglb+numglbs*symsiz
#define	startloc endglb+symsiz
#define	endloc	symtab+symtbsz-symsiz


/*	Define symbol table entry format	*/


#define	name	0
#define	ident	9
#define	type	10
#define	storage	11
#define	offset	12


/*	System wide name size (for symbols)	*/


#define	namesize 9
#define namemax  8


/*	Define data for external symbols	*/

#define extblsz 2000
#define startextrn exttab
#define endextrn exttab+extblsz-namesize-1

/* Possible types of exttab entries */
/* Stored in the byte following zero terminating the name */

#define userfunc 1
#define statref 2


/*	Define possible entries for "ident"	*/


#define	variable 1
#define	array	2
#define	pointer	3
#define	function 4


/*	Define possible entries for "type"	*/


#define	cchar	1
#define	cint	2


/*	Define possible entries for "storage"	*/


#define	statik	1
#define	stkloc	2


/*	Define the "while" statement queue	*/


#define	wqtabsz	100
#define	wqsiz	4
#define	wqmax	wq+wqtabsz-wqsiz


/*	Define entry offsets in while queue	*/


#define	wqsym	0
#define	wqsp	1
#define	wqloop	2
#define	wqlab	3


/*	Define the literal pool			*/


#define	litabsz	3000
#define	litmax	litabsz-1


/*	Define the input line			*/


#define	linesize 80
#define	linemax	linesize-1
#define	mpmax	linemax


/*	Define the macro (define) pool		*/


#define	macqsize 1000
#define	macmax	macqsize-1


/*	Define statement types (tokens)		*/


#define	stif	1
#define	stwhile	2
#define	streturn 3
#define	stbreak	4
#define	stcont	5
#define	stasm	6
#define	stexp	7


/*	Now reserve some storage words		*/


char	exttab[extblsz];	/* external symbols */
char	*extptr;		/* pointer to next available entry */


char	symtab[symtbsz];	/* symbol table */
char	*glbptr,*locptr;		/* ptrs to next entries */


int	wq[wqtabsz];		/* while queue */
int	*wqptr;			/* ptr to next entry */


char	litq[litabsz];		/* literal pool */
int	litptr;			/* ptr to next entry */


char	macq[macqsize];		/* macro string buffer */
int	macptr;			/* and its index */


char	line[linesize];		/* parsing buffer */
char	mline[linesize];	/* temp macro buffer */
int	lptr,mptr;		/* ptrs into each */


/*	Misc storage	*/


int	nxtlab,		/* next avail label # */
	litlab,		/* label # assigned to literal pool */
	cextern,	/* collecting external names flag */
	Zsp,		/* compiler relative stk ptr */
	argstk,		/* function arg sp */
	errcnt,		/* # errors in compilation */
	errstop,	/* stop on error			gtf 7/17/80 */
	eof,		/* set non-zero on final input eof */
	input,		/* iob # for input file */
	output,		/* iob # for output file (if any) */
	input2,		/* iob # for "include" file */
	ctext,		/* non-zero to intermix c-source */
	cmode,		/* non-zero while parsing c-code */
			/* zero when passing assembly code */
	lastst,		/* last executed statement type */
	saveout,	/* holds output ptr when diverted to console	   */
			/*					gtf 7/16/80 */
	lineno,		/* line# in current file		gtf 7/2/80 */
	saveline;	/* copy of lineno  "	"		gtf 7/16/80 */


char   *currfn;		/* ptr to symtab entry for current fn.	gtf 7/17/80 */
char	quote[2];	/* literal string for '"' */
char	*cptr;		/* work ptr to any char buffer */


/*					*/
/*	Compiler begins execution here	*/
/*					*/
main()
{
	hello();	/* greet user */
	see();		/* determine options */
	openin();
	openout();
	litlab=1;
	extptr=startextrn;	/* clear external symbols */
	glbptr=startglb;	/* clear global symbols */
	wqptr=wq;		/* clear while queue */
	macptr=		/* clear the macro pool */
	litptr=		/* clear literal pool */
	errcnt=		/* no errors */
	eof=		/* not end-of-file yet */
	input2=		/* no include file */
	saveout=	/* no diverted output */
	lastst=		/* no last statement yet */
	cextern=	/* no externs yet */
	lineno=		/* no lines read from file		gtf 7/2/80 */
	quote[1]=
		0;		/*  ...all set to zero.... */
	quote[0]='"';		/* fake a quote literal */
	currfn=NULL;	/* no function yet			gtf 7/2/80 */
	cmode=nxtlab=1;	/* enable preprocessing and reset label numbers */
	header();
	parse();
	extdump();
	dumpublics();
	trailer();
	closeout();
	errorsummary();
}
/* ### cpcn-2 */
/*					*/
/*	Abort compilation		*/
/*		gtf 7/17/80		*/
abort()
{
	if(input2)
		endinclude();
	if(input)
		fclose(input);
	closeout();
	toconsole();
	pl("Compilation aborted.");  nl();
	exit();
/* end abort */}


/*					*/
/*	Process all input text		*/
/*					*/
/* At this level, only static declarations, */
/*	defines, includes, and function */
/*	definitions are legal...	*/
parse()
	{
	while (eof==0)		/* do until no more input */
		{
		if(amatch("extern",6)) {
			cextern=1;
			if(amatch("char",4)) {declglb(cchar);ns();}
			else if(amatch("int",3)) {declglb(cint);ns();}
			else {declglb(cint);ns();}
			cextern=0;
			}
		else if(amatch("char",4)){declglb(cchar);ns();}
		else if(amatch("int",3)){declglb(cint);ns();}
		else if(match("#asm"))doasm();
		else if(match("#include"))doinclude();
		else if(match("#define"))addmac();
		else newfunc();
		blanks();	/* force eof if pending */
		}
	}
/* ### cpcn-3 */
dumpublics()
	{
	outstr("DUMMY SEGMENT BYTE STACK 'dummy'");nl();
	outstr("DUMMY ENDS");nl();
	outstr("STACK SEGMENT BYTE PUBLIC 'stack'");nl();
	dumplits();
	dumpglbs();
	outstr("STACK ENDS");nl();
	}

extdump()
	{
	char *ptrext;
	ptrext=startextrn;
	while(ptrext<extptr)
		{
		if((cptr=findglb(ptrext))!=0)
			{if(cptr[ident]==function)
				if(cptr[offset]!=function) outextrn(ptrext);
			}
		else outextrn(ptrext);
		ptrext=ptrext+strlen(ptrext)+2;
		}
	outstr("CSEG ENDS");nl();
	}
/* ### cpcn-4 */
outextrn(ptr)
	char *ptr;
{
	char *functype;
	functype=ptr+strlen(ptr)+1;
	if(*functype==statref) return;
	ot("EXTRN ");
	outname(ptr);
	outstr(":NEAR");nl();
}


/*					*/
/*	Dump the literal pool		*/
/*					*/
dumplits()
	{int j,k;
	if (litptr==0) return;	/* if nothing there, exit...*/
	printlabel(litlab); /* print literal label */
	k=0;			/* init an index... */
	while (k<litptr)	/* 	to loop with */
		{defbyte();	/* pseudo-op to define byte */
		j=10;		/* max bytes per line */
		while(j--)
			{outdec((litq[k++]));
			if ((j==0) | (k>=litptr))
				{nl();		/* need <cr> */
				break;
				}
			outbyte(',');	/* separate bytes */
			}
		}
	}
/*					*/
/*	Dump all static variables	*/
/*					*/
dumpglbs()
{
	int j;
	cptr=startglb;
	while(cptr<glbptr)
		{if(cptr[ident]!=function)
			/* do if anything but function */
			{
			if(findext(cptr+name))
				{
				ot("EXTRN ");
				outname(cptr);col();
				if((cptr[type]==cint) |
					(cptr[ident]==pointer)) outstr("WORD");
				else outstr("BYTE");
				nl();
				}
			else {
				ot("PUBLIC ");outname(cptr);nl();
				outname(cptr);
				/* output name as label... */
				j=(cptr[offset]&255)+
					(cptr[offset+1]<<8);
					/* calc # bytes */
				if((cptr[type]==cint)|
					(cptr[ident]==pointer)) defword();
				else defbyte();
				outdec(j);
				outstr(" DUP(?)");
				nl();
				}
			}
		cptr=cptr+symsiz;
		}
}
/*					*/
/*	Report errors for user		*/
/*					*/
errorsummary()
	{
	nl();
	outstr("There were ");
	outdec(errcnt);	/* total # errors */
	outstr(" errors in compilation.");
	nl();
	}
/* ### cpcn-5 */
/*	Greet User	*/


hello()
{
	nl();nl();		/* print banner */
	pl(BANNER);
	nl();
	pl(AUTHOR);
	nl();
	pl(VERSION);
	nl();
	nl();
}


see()
	{
	kill();
	/* see if user wants to be sure to see all errors */
	pl("Should I pause after an error (y,N)?");
	gets(line);
	errstop=0;
	if((ch()=='Y')|(ch()=='y'))
		errstop=1;

	kill();
	pl("Do you want the small-c:PC-text to appear (y,N)?");
	gets(line);
	ctext=0;
	if((ch()=='Y')|(ch()=='y')) ctext=1;
        }
/*					*/
/*	Get output filename		*/
/*					*/
openout()
{
	output=0;		/* start with none */
	while(output==0){
		pl("Output filename? "); /* ask...*/
		gets(line);	/* get a filename */
		if ((output=fopen(line,"w"))==NULL) {
			pl("Open failure!");
		}
	}
	kill();			/* erase line */
}
/*					*/
/*	Get (next) input file		*/
/*					*/
openin()
{
	input=0;		/* none to start with */
	while(input==0){	/* any above 1 allowed */
		pl("Input filename? ");
		gets(line);	/* get a name */
		if ((input=fopen(line,"r"))==NULL) {
			pl("Open failure");
		}
	}
}
/*					*/
/*	Open an include file		*/
/*					*/
doinclude()
{
	blanks();	/* skip over to name */
	toconsole();					/* gtf 7/16/80 */
	outstr("#include "); outstr(line+lptr); nl();
	tofile();
	if (input2) {
		error("Cannot nest include files");
	}
	else if ((input2=fopen(line+lptr,"r"))==NULL) {
		input2=0;
		error("Open failure on include file");
	}
	else {
		saveline = lineno;
		lineno = 0;
	}
	kill();		/* clear rest of line */
			/* so next read will come from */
			/* new file (if open */
}


/*					*/
/*	Close an include file		*/
/*			gtf 7/16/80	*/
endinclude()
{
	toconsole();
	outstr("#end include"); nl();
	tofile();
	input2  = 0;
	lineno  = saveline;
}


/*					*/
/*	Close the output file		*/
/*					*/
closeout()
{
	tofile();	/* if diverted, return to file */
	if(output)fclose(output); /* if open, close it */
	output=0;		/* mark as closed */
}
/* ### cpcn-7 */
/*					*/
/*	Declare a static variable	*/
/*	  (i.e. define for use)		*/
/*					*/
/* makes an entry in the symbol table so subsequent */
/*  references can call symbol by name	*/
declglb(typ)		/* typ is cchar or cint */
	int typ;
{	int k,j;char sname[namesize];
	while(1)
		{while(1)
			{if(endst())return;	/* do line */
			k=1;		/* assume 1 element */
			if(match("*"))	/* pointer ? */
				j=pointer;	/* yes */
				else j=variable; /* no */
			 if (symname(sname)==0) /* name ok? */
				illname(); /* no... */
			if(findglb(sname)) /* already there? */
				multidef(sname);
			if (match("["))		/* array? */
				{k=needsub();	/* get size */
				if(k)j=array;	/* !0=array */
				else j=pointer; /* 0=ptr */
				}
			addglb(sname,j,typ,k); /* add symbol */
			if(cextern) addext(sname,statref);
			break;
			}
		if (match(",")==0) return; /* more? */
		}
	}
/*					*/
/*	Declare local variables		*/
/*	(i.e. define for use)		*/
/*					*/
/* works just like "declglb" but modifies machine stack */
/*	and adds symbol table entry with appropriate */
/*	stack offset to find it again			*/
declloc(typ)		/* typ is cchar or cint */
	int typ;
	{
	int k,j;char sname[namesize];
	while(1)
		{while(1)
			{if(endst())return;
			if(match("*"))
				j=pointer;
				else j=variable;
			if (symname(sname)==0)
				illname();
			if(findloc(sname))
				multidef(sname);
			if (match("["))
				{k=needsub();
				if(k)
					{j=array;
					if(typ==cint)k=k+k;
					}
				else
					{j=pointer;
					k=2;
					}
				}
			else
				if((typ==cchar)
					&(j!=pointer))
					k=1;else k=2;
			/* change machine stack */
			Zsp=modstk(Zsp-k);
			addloc(sname,j,typ,Zsp);
			break;
			}
		if (match(",")==0) return;
		}
	}
/* ### cpcn-8 */
/*	>>>>>> start of cc2 <<<<<<<<	*/


/*					*/
/*	Get required array size		*/
/*					*/
/* invoked when declared variable is followed by "[" */
/*	this routine makes subscript the absolute */
/*	size of the array. */
needsub()
	{
	int num[1];
	if(match("]"))return 0;	/* null size */
	if (number(num)==0)	/* go after a number */
		{error("must be constant");	/* it isn't */
		num[0]=1;		/* so force one */
		}
	if (num[0]<0)
		{error("negative size illegal");
		num[0]=(-num[0]);
		}
	needbrack("]");		/* force single dimension */
	return num[0];		/* and return size */
	}
/*					*/
/* ### cpcn-9 */
/*	Begin a function		*/
/*					*/
/* Called from "parse" this routine tries to make a function */
/*	out of what follows.	*/
newfunc()
	{
	char n[namesize]; /* ptr => currfn,  gtf 7/16/80 */
	if (symname(n)==0)
		{
		if(eof==0) error("illegal function or declaration");
		kill();	/* invalidate line */
		return;
		}
	if(currfn=findglb(n))	/* already in symbol table ? */
		{if(currfn[ident]!=function)multidef(n);
			/* already variable by that name */
		else if(currfn[offset]==function)multidef(n);
			/* already function by that name */
		else currfn[offset]=function;
			/* otherwise we have what was earlier*/
			/*  assumed to be a function */
		}
	/* if not in table, define as a function now */
	else currfn=addglb(n,function,cint,function);


	toconsole();					/* gtf 7/16/80 */
	outstr("====== "); outstr(currfn+name); outstr("()"); nl();
	tofile();


	/* we had better see open paren for args... */
	if(match("(")==0)error("missing open paren");
	ot("PUBLIC ");outname(n);nl();
	outname(n);col();nl();	/* print function name */
	argstk=0;		/* init arg count */
	while(match(")")==0)	/* then count args */
		/* any legal name bumps arg count */
		{if(symname(n))argstk=argstk+2;
		else{error("illegal argument name");junk();}
		blanks();
		/* if not closing paren, should be comma */
		if(streq(line+lptr,")")==0)
			{if(match(",")==0)
			error("expected comma");
			}
		if(endst())break;
		}
	locptr=startloc;	/* "clear" local symbol table*/
	Zsp=0;			/* preset stack ptr */
	while(argstk)
		/* now let user declare what types of things */
		/*	those arguments were */
		{if(amatch("char",4)){getarg(cchar);ns();}
		else if(amatch("int",3)){getarg(cint);ns();}
		else{error("wrong number args");break;}
		}
	if(statement()!=streturn) /* do a statement, but if */
				/* it's a return, skip */
				/* cleaning up the stack */
		{modstk(0);
		zret();
		}
}
/* ### cpcn-10 */
/*					*/
/*	Declare argument types		*/
/*					*/
/* called from "newfunc" this routine adds an entry in the */
/*	local symbol table for each named argument */
getarg(t)		/* t = cchar or cint */
	int t;
	{
	char n[namesize],c;int j;
	while(1)
		{if(argstk==0)return;	/* no more args */
		if(match("*"))j=pointer;
			else j=variable;
		if(symname(n)==0) illname();
		if(findloc(n))multidef(n);
		if(match("["))	/* pointer ? */
		/* it is a pointer, so skip all */
		/* stuff between "[]" */
			{while(inbyte()!=']')
				if(endst())break;
			j=pointer;
			/* add entry as pointer */
			}
		addloc(n,j,t,argstk);
		argstk=argstk-2;	/* cnt down */
		if(endst())return;
		if(match(",")==0)error("expected comma");
		}
	}
/* ### cpcn-11 */
/*	Statement parser		*/
/*					*/
/* called whenever syntax requires	*/
/*	a statement. 			 */
/*  this routine performs that statement */
/*  and returns a number telling which one */
statement()
{
/* comment out ctrl-C check since ctrl-break will work on PC */
/*	if(cpm(11,0) & 1) */	/* check for ctrl-C		gtf 7/17/80 */
	/*	if(getchar()==3) */
		/*	abort(); */


	if ((ch()==0) & (eof)) return;
	else if(amatch("char",4))
		{declloc(cchar);ns();}
	else if(amatch("int",3))
		{declloc(cint);ns();}
	else if(match("{"))compound();
	else if(amatch("if",2))
		{doif();lastst=stif;}
	else if(amatch("while",5))
		{dowhile();lastst=stwhile;}
	else if(amatch("return",6))
		{doreturn();ns();lastst=streturn;}
	else if(amatch("break",5))
		{dobreak();ns();lastst=stbreak;}
	else if(amatch("continue",8))
		{docont();ns();lastst=stcont;}
	else if(match(";"));
	else if(match("#asm"))
		{doasm();lastst=stasm;}
	/* if nothing else, assume it's an expression */
	else{expression();ns();lastst=stexp;}
	return lastst;
}
/*					*/
/* ### cpcn-12 */
/*	Semicolon enforcer		*/
/*					*/
/* called whenever syntax requires a semicolon */
ns()	{if(match(";")==0)error("missing semicolon");}
/*					*/
/*	Compound statement		*/
/*					*/
/* allow any number of statements to fall between "{}" */
compound()
{
	while (match("}")==0) statement(); /* do one */
}
/*					*/
/*		"if" statement		*/
/*					*/
doif()
	{
	int flev,fsp,flab1,flab2;
	flev=locptr;	/* record current local level */
	fsp=Zsp;		/* record current stk ptr */
	flab1=getlabel(); /* get label for false branch */
	test(flab1);	/* get expression, and branch false */
	statement();	/* if true, do a statement */
	Zsp=modstk(fsp);	/* then clean up the stack */
	locptr=flev;	/* and deallocate any locals */
	if (amatch("else",4)==0)	/* if...else ? */
		/* simple "if"...print false label */
		{printlabel(flab1);col();nl();
		return;		/* and exit */
		}
	/* an "if...else" statement. */
	jump(flab2=getlabel());	/* jump around false code */
	printlabel(flab1);col();nl();	/* print false label */
	statement();		/* and do "else" clause */
	Zsp=modstk(fsp);		/* then clean up stk ptr */
	locptr=flev;		/* and deallocate locals */
	printlabel(flab2);col();nl();	/* print true label */
	}
/*					*/
/*	"while" statement		*/
/*					*/
dowhile()
	{
	int wq[4];		/* allocate local queue */
	wq[wqsym]=locptr;	/* record local level */
	wq[wqsp]=Zsp;		/* and stk ptr */
	wq[wqloop]=getlabel();	/* and looping label */
	wq[wqlab]=getlabel();	/* and exit label */
	addwhile(wq);		/* add entry to queue */
				/* (for "break" statement) */
	printlabel(wq[wqloop]);col();nl(); /* loop label */
	test(wq[wqlab]);	/* see if true */
	statement();		/* if so, do a statement */
	Zsp = modstk(wq[wqsp]);	/* zap local vars: 9/25/80 gtf */
	jump(wq[wqloop]);	/* loop to label */
	printlabel(wq[wqlab]);col();nl(); /* exit label */
	locptr=wq[wqsym];	/* deallocate locals */
	delwhile();		/* delete queue entry */
	}
/*					*/
/* ### cpcn-13 */
/*					*/
/*	"return" statement		*/
/*					*/
doreturn()
	{
	/* if not end of statement, get an expression */
	if(endst()==0)expression();
	modstk(0);	/* clean up stk */
	zret();		/* and exit function */
	}
/*					*/
/*	"break" statement		*/
/*					*/
dobreak()
	{
	int *ptr;
	/* see if any "whiles" are open */
	if ((ptr=readwhile())==0) return;	/* no */
	modstk((ptr[wqsp]));	/* else clean up stk ptr */
	jump(ptr[wqlab]);	/* jump to exit label */
	}
/*					*/
/*	"continue" statement		*/
/*					*/
docont()
	{
	int *ptr;
	/* see if any "whiles" are open */
	if ((ptr=readwhile())==0) return;	/* no */
	modstk((ptr[wqsp]));	/* else clean up stk ptr */
	jump(ptr[wqloop]);	/* jump to loop label */
	}
/*					*/
/*	"asm" pseudo-statement		*/
/*					*/
/* enters mode where assembly language statement are */
/*	passed intact through parser	*/
doasm()
	{
	cmode=0;		/* mark mode as "asm" */
	while (1)
		{inline();	/* get and print lines */
		if (match("#endasm")) break;	/* until... */
		if(eof)break;
		outstr(line);
		nl();
		}
	kill();		/* invalidate line */
	cmode=1;		/* then back to parse level */
	}
/* ### cpcn-14 */
/*	>>>>> start of cc3 <<<<<<<<<	*/


/*					*/
/*	Perform a function call		*/
/*					*/
/* called from heir11, this routine will either call */
/*	the named function, or if the supplied ptr is */
/*	zero, will call the contents of BX		*/
callfunction(ptr)
	char *ptr;	/* symbol table entry (or 0) */
{	int nargs;
	nargs=0;
	blanks();	/* already saw open paren */
	if(ptr==0)zpush();	/* calling BX */
	while(streq(line+lptr,")")==0)
		{if(endst())break;
		expression();	/* get an argument */
		if(ptr==0)swapstk(); /* don't push addr */
		zpush();	/* push argument */
		nargs=nargs+2;	/* count args*2 */
		if (match(",")==0) break;
		}
	needbrack(")");
	if(ptr)zcall(ptr);
	else callstk();
	Zsp=modstk(Zsp+nargs);	/* clean up arguments */
}
junk()
{	if(an(inbyte()))
		while(an(ch()))gch();
	else while(an(ch())==0)
		{if(ch()==0)break;
		gch();
		}
	blanks();
}
endst()
{	blanks();
	return ((streq(line+lptr,";")|(ch()==0)));
}
illname()
{	error("illegal symbol name");junk();}
multidef(sname)
	char *sname;
{	error("already defined");
	comment();
	outstr(sname);nl();
}
needbrack(str)
	char *str;
{	if (match(str)==0)
		{error("missing bracket");
		comment();outstr(str);nl();
		}
}
needlval()
{	error("must be lvalue");
}
/* ### cpcn-15 */
findglb(sname)
	char *sname;
{	char *ptr;
	ptr=startglb;
	while(ptr!=glbptr)
		{if(astreq(sname,ptr,namemax))return ptr;
		ptr=ptr+symsiz;
		}
	return 0;
}
findloc(sname)
	char *sname;
{	char *ptr;
	ptr=startloc;
	while(ptr!=locptr)
		{if(astreq(sname,ptr,namemax))return ptr;
		ptr=ptr+symsiz;
		}
	return 0;
}
addglb(sname,id,typ,value)
	char *sname,id,typ;
	int value;
{	char *ptr;
	if(cptr=findglb(sname))return cptr;
	if(glbptr>=endglb)
		{error("global symbol table overflow");
		return 0;
		}
	cptr=ptr=glbptr;
	while(an(*ptr++ = *sname++));	/* copy name */
	cptr[ident]=id;
	cptr[type]=typ;
	cptr[storage]=statik;
	cptr[offset]=value;
	cptr[offset+1]=value>>8;
	glbptr=glbptr+symsiz;
	return cptr;
}
addloc(sname,id,typ,value)
	char *sname,id,typ;
	int value;
{	char *ptr;
	if(cptr=findloc(sname))return cptr;
	if(locptr>=endloc)
		{error("local symbol table overflow");
		return 0;
		}
	cptr=ptr=locptr;
	while(an(*ptr++ = *sname++));	/* copy name */
	cptr[ident]=id;
	cptr[type]=typ;
	cptr[storage]=stkloc;
	cptr[offset]=value;
	cptr[offset+1]=value>>8;
	locptr=locptr+symsiz;
	return cptr;
}
/* ### cpcn-16 */
addext(sname,id)
	char *sname,id;
	{
	char *ptr;
	if(cptr=findext(sname)) return cptr;
	if(extptr>=endextrn)
		{error("external symbol table overflow"); return 0;}
	cptr=ptr=extptr;
	while(an(*ptr++=*sname++)); /* copy name */
	/* type stored in byte following zero terminating name */
	*ptr++=id;
	extptr=ptr;
	return cptr;
	}
findext(sname)
	char *sname;
	{char *ptr;
	ptr=startextrn;
	while(ptr<extptr)
		{if(astreq(sname,ptr,namemax)) return ptr;
		 ptr=ptr+strlen(ptr)+2;
		}
	return 0;
	}


/* Test if next input string is legal symbol name */
symname(sname)
	char *sname;
{	int k;
	blanks();
	if(alpha(ch())==0)return 0;
	k=0;
	while(an(ch()))sname[k++]=gch();
	sname[k]=0;
	return 1;
	}
/* ### cpcn-17 */
/* Return next avail internal label number */
getlabel()
{	return(++nxtlab);
}
/* Print specified number as label */
printlabel(label)
	int label;
{	outasm("cc");
	outdec(label);
}
/* Test if given character is alpha */
alpha(c)
	int c;
{
	return(((c>='a')&(c<='z'))|
		((c>='A')&(c<='Z'))|
		(c=='_'));
}
/* Test if given character is numeric */
numeric(c)
	int c;
{
	return((c>='0')&(c<='9'));
}
/* Test if given character is alphanumeric */
an(c)
	int c;
{
	return alpha(c)|numeric(c);
}
/* Print a carriage return and a string only to console */
pl(str)
	char *str;
{	int k;
	k=0;
	putchar(eol);
	while(str[k])putchar(str[k++]);
}
addwhile(ptr)
	int ptr[];
 {
	int k;
	if (wqptr==wqmax)
		{error("too many active whiles");return;}
	k=0;
	while (k<wqsiz)
		{*wqptr++ = ptr[k++];}
}
delwhile()
	{if(readwhile()) wqptr=wqptr-wqsiz;
	}
readwhile()
 {
	if (wqptr==wq){error("no active whiles");return 0;}
	else return (wqptr-wqsiz);
 }
ch()
{	return line[lptr];
}
nch()
{	if(ch()==0)return 0;
	else return line[lptr+1];
}
gch()
{	if(ch()==0)return 0;
	else return line[lptr++];
}
kill()
{	lptr=0;
	line[lptr]=0;
}
/* ### cpcn-18 */
inbyte()
{
	while(ch()==0)
		{if (eof) return 0;
		inline();
		preprocess();
		}
	return gch();
}
inline()
{
	int k,unit;
	while(1)
		{if (input==0)  {eof=1;return;}
		if((unit=input2)==0)unit=input;
		kill();
		while((k=getc(unit))>0)
			{if((k==eol)|(lptr>=linemax))break;
			line[lptr++]=k;
			}
		line[lptr]=0;	/* append null */
		lineno++;	/* read one more line		gtf 7/2/80 */
		if(k<=0)
			{fclose(unit);
			if(input2)endinclude();		/* gtf 7/16/80 */
				else input=0;
			}
		if(lptr)
			{if((ctext)&(cmode))
				{comment();
				outstr(line);
				nl();
				}
			lptr=0;
			return;
			}
		}
}
/* ### cpcn-19 */
/*	>>>>>> start of cc4 <<<<<<<	*/


preprocess()
{	int c,k;
	char sname[namesize];
	if(cmode==0)return;
	mptr=lptr=0;
	while(ch())
		{if((ch()==' ')|(ch()==9))
			predel();
		else if(ch()=='"')
			prequote();
		else if(ch()==39)
			preapos();
		else if((ch()=='/')&(nch()=='*'))
			precomm();
		else if(alpha(ch()))	/* from an(): 9/22/80 gtf */
			{k=0;
			while(an(ch()))
				{if(k<namemax)sname[k++]=ch();
				gch();
				}
			sname[k]=0;
			if(k=findmac(sname))
				while(c=macq[k++])
					keepch(c);
			else
				{k=0;
				while(c=sname[k++])
					keepch(c);
				}
			}
		else keepch(gch());
		}
	keepch(0);
	if(mptr>=mpmax)error("line too long");
	lptr=mptr=0;
	while(line[lptr++]=mline[mptr++]);
	lptr=0;
	}
/* ### cpcn-20 */
keepch(c)
	int c;
{	mline[mptr]=c;
	if(mptr<mpmax)mptr++;
	return c;
}
predel()
			{keepch(' ');
			while((ch()==' ')|
				(ch()==9))
				gch();
			}
prequote()
			{keepch(ch());
			gch();
			while(ch()!='"')
				{if(ch()==0)
				  {error("missing quote");
				  break;
				  }
				keepch(gch());
				}
			gch();
			keepch('"');
			}
preapos()
			{keepch(39);
			gch();
			while(ch()!=39)
				{if(ch()==0)
				  {error("missing apostrophe");
				  break;
				  }
				keepch(gch());
				}
			gch();
			keepch(39);
			}
precomm()
			{gch();gch();
			while(((ch()=='*')&
				(nch()=='/'))==0)
				{if(ch()==0)inline();
					else gch();
				if(eof)break;
				}
			gch();gch();
			}
/* ### cpcn-21 */
addmac()
{	char sname[namesize];
	int k;
	if(symname(sname)==0)
		{illname();
		kill();
		return;
		}
	k=0;
	while(putmac(sname[k++]));
	while(ch()==' ' | ch()==9) gch();
	while(putmac(gch()));
	if(macptr>=macmax)error("macro table full");
	}
putmac(c)
	int c;
{	macq[macptr]=c;
	if(macptr<macmax)macptr++;
	return c;
}
findmac(sname)
	char *sname;
{	int k;
	k=0;
	while(k<macptr)
		{if(astreq(sname,macq+k,namemax))
			{while(macq[k++]);
			return k;
			}
		while(macq[k++]);
		while(macq[k++]);
		}
	return 0;
}
/* ### cpcn-22 */
/* direct output to console		gtf 7/16/80 */
toconsole()
{
	saveout = output;
	output = 0;
/* end toconsole */}


/* direct output back to file		gtf 7/16/80 */
tofile()
{
	if(saveout)
		output = saveout;
	saveout = 0;
/* end tofile */}


outbyte(c)
	int c;
{
	if(c==0)return 0;
	if(output)
		{if((putc(c,output))<=0)
			{closeout();
			error("Output file error");
			abort();			/* gtf 7/17/80 */
			}
		}
	else putchar(c);
	return c;
}

outstr(ptr)
	char ptr[];
{
	while(outbyte(*ptr++));
}

outasm(ptr)
	char *ptr;
{
	while(outbyte(raise(*ptr++)));
}

nl()
	{outbyte(eol);}
tab()
	{outbyte(9);}
col()
	{outbyte(58);}

error(ptr)
	char ptr[];
{
	int k;
	char junk[81];
	toconsole();
	outstr("Line ");outdec(lineno);
	outstr(": ");outstr(ptr);nl();
	outstr(line); nl();
	++errcnt;
	if (errstop) {
		pl("Continue (Y,n,g) ? ");
		gets(junk);		
		k=junk[0];
		if((k=='N') | (k=='n'))
			abort();
		if((k=='G') | (k=='g'))
			errstop=0;
	}
	tofile();
}


ol(ptr)
	char ptr[];
{
	ot(ptr);
	nl();
}
ot(ptr)
	char ptr[];
{
	tab();
	outasm(ptr);
}
/* ### cpcn-24 */
streq(str1,str2)
	char str1[],str2[];
 {
	int k;
	k=0;
	while (str2[k])
		{if ((str1[k])!=(str2[k])) return 0;
		k++;
		}
	return k;
 }
astreq(str1,str2,len)
	char str1[],str2[];int len;
 {
	int k;
	k=0;
	while (k<len)
		{if ((str1[k])!=(str2[k]))break;
		if(str1[k]==0)break;
		if(str2[k]==0)break;
		k++;
		}
	if (an(str1[k]))return 0;
	if (an(str2[k]))return 0;
	return k;
 }
match(lit)
	char *lit;
{
	int k;
	blanks();
	if (k=streq(line+lptr,lit))
		{lptr=lptr+k;
		return 1;
		}
 	return 0;
}
amatch(lit,len)
	char *lit;int len;
 {
	int k;
	blanks();
	if (k=astreq(line+lptr,lit,len))
		{lptr=lptr+k;
		while(an(ch())) inbyte();
		return 1;
		}
	return 0;
 }
/* ### cpcn-25 */
blanks()
	{while(1)
		{while(ch()==0)
			{inline();
			preprocess();
			if(eof)break;
			}
		if(ch()==' ')gch();
		else if(ch()==9)gch();
		else return;
		}
	}
outdec(number)
	int number;
 {
	int c,k,zs;
	zs = 0;
	k=10000;
	if (number<0)
		{number=(-number);
		outbyte('-');
		}
	while (k>=1)
		{
		c=number/k + '0';
		if ((c!='0')|(k==1)|(zs))
			{zs=1;outbyte(c);}
		number=number%k;
		k=k/10;
		}
 }
/* return the length of a string */
strlen(s)
	char *s;
{
	char *t;
	t = s;
	while(*s) s++;
	return s-t;
}


/* convert lower case to upper */
raise(c)
	int c;
{
	if((c>='a') & (c<='z'))
		c = c - 'a' + 'A';
	return c;
}

expression()
{
	int lval[2];
	if(heir1(lval))rvalue(lval);
}
heir1(lval)
	int lval[];
{
	int k,lval2[2];
	k=heir2(lval);
	if (match("="))
		{if(k==0){needlval();return 0;}
		if (lval[1])zpush();
		if(heir1(lval2))rvalue(lval2);
		store(lval);
		return 0;
		}
	else return k;
}
heir2(lval)
	int lval[];
{	int k,lval2[2];
	k=heir3(lval);
	if(ch()!='|')return k;
	if(k)rvalue(lval);
	while(1)
		{if (match("|"))
			{zpush();
			if(heir3(lval2)) rvalue(lval2);
			zpop();
			zor();
			}
		else return 0;
		}
}
heir3(lval)
	int lval[];
{	int k,lval2[2];
	k=heir4(lval);
	if(ch()!='^')return k;
	if(k)rvalue(lval);
	while(1)
		{if (match("^"))
			{zpush();
			if(heir4(lval2))rvalue(lval2);
			zpop();
			zxor();
			}
		else return 0;
		}
}
heir4(lval)
	int lval[];
{	int k,lval2[2];
	k=heir5(lval);
	if(ch()!='&')return k;
	if(k)rvalue(lval);
	while(1)
		{if (match("&"))
			{zpush();
			if(heir5(lval2))rvalue(lval2);
			zpop();
			zand();
			}
		else return 0;
		}
}
heir5(lval)
	int lval[];
{
	int k,lval2[2];
	k=heir6(lval);
	if((streq(line+lptr,"==")==0)&
		(streq(line+lptr,"!=")==0))return k;
	if(k)rvalue(lval);
	while(1)
		{if (match("=="))
			{zpush();
			if(heir6(lval2))rvalue(lval2);
			zpop();
			zeq();
			}
		else if (match("!="))
			{zpush();
			if(heir6(lval2))rvalue(lval2);
			zpop();
			zne();
			}
		else return 0;
		}
}
heir6(lval)
	int lval[];
{
	int k;
	k=heir7(lval);
	if((ch()!='<')&(ch()!='>'))return k;
	if(k)rvalue(lval);
	while(1)
		{if (match("<="))
			{if(heir6wrk(lval)) ule();
			else zle();
		}
		else if (match(">="))
			{if(heir6wrk(lval)) uge();
			else zge();
		}
		else if (match("<"))
			{if(heir6wrk(lval)) ult();
			else zlt();
		}
		else if (match(">"))
			{if(heir6wrk(lval)) ugt();
			else zgt();
		}
		else return 0;
	}
}
heir6wrk(lval)
	int lval[];
{
	int lval2[2];
	zpush();
	if(heir7(lval2))rvalue(lval2);
	zpop();
	if(cptr=lval[0])
		if(cptr[ident]==pointer)
			return 1;
	if(cptr=lval2[0])
		if(cptr[ident]==pointer)
			return 1;
	return 0;
}
heir7(lval)
	int lval[];
{
	int k,lval2[2];
	k=heir8(lval);
	if((streq(line+lptr,">>")==0)&
		(streq(line+lptr,"<<")==0))return k;
	if(k)rvalue(lval);
	while(1)
		{if (match(">>"))
			{zpush();
			if(heir8(lval2))rvalue(lval2);
			zpop();
			asr();
			}
		else if (match("<<"))
			{zpush();
			if(heir8(lval2))rvalue(lval2);
			zpop();
			asl();
			}
		else return 0;
		}
}
heir8(lval)
	int lval[];
{
	int k,lval2[2];
	k=heir9(lval);
	if((ch()!='+')&(ch()!='-'))return k;
	if(k)rvalue(lval);
	while(1)
		{if (match("+"))
			{zpush();
			if(heir9(lval2))rvalue(lval2);
			if(cptr=lval[0])
				if((cptr[ident]==pointer)&
				(cptr[type]==cint))
				doublereg();
			zpop();
			zadd();
			}
		else if (match("-"))
			{zpush();
			if(heir9(lval2))rvalue(lval2);
			if(cptr=lval[0])
				if((cptr[ident]==pointer)&
				(cptr[type]==cint))
				doublereg();
			zpop();
			zsub();
			}
		else return 0;
		}
}
heir9(lval)
	int lval[];
{
	int k,lval2[2];
	k=heir10(lval);
	blanks();
	if((ch()!='*')&(ch()!='/')&
		(ch()!='%'))return k;
	if(k)rvalue(lval);
	while(1)
		{if (match("*"))
			{zpush();
			if(heir9(lval2))rvalue(lval2);
			zpop();
			mult();
			}
		else if (match("/"))
			{zpush();
			if(heir10(lval2))rvalue(lval2);
			zpop();
			div();
			}
		else if (match("%"))
			{zpush();
			if(heir10(lval2))rvalue(lval2);
			zpop();
			zmod();
			}
		else return 0;
		}
}
/* ### cpcn-33 */
heir10(lval)
	int lval[];
{
	int k;
	if(match("++"))
		{if((k=heir10(lval))==0)
			{needlval();
			return 0;
			}
		heir10inc(lval);
		return 0;
		}
	else if(match("--"))
		{if((k=heir10(lval))==0)
			{needlval();
			return 0;
			}
		heir10dec(lval);
		return 0;
		}
	else if (match("-"))
		{k=heir10(lval);
		if (k) rvalue(lval);
		neg();
		return 0;
		}
	else if(match("*"))
		{heir10as(lval);
		return 1;
		}
	else if(match("&"))
		{k=heir10(lval);
		if(k==0)
			{error("illegal address");
			return 0;
			}
		else if(lval[1])return 0;
		else
			{heir10at(lval);
			return 0;
			}
		}
	else 
		{k=heir11(lval);
		if(match("++"))
			{if(k==0)
				{needlval();
				return 0;
				}
			heir10id(lval);
			return 0;
			}
		else if(match("--"))
			{if(k==0)
				{needlval();
				return 0;
				}
			heir10di(lval);
			return 0;
			}
		else return k;
		}
	}
/* ### cpcn-34 */
heir10inc(lval)
	int lval[];
{
	char *ptr;
	if(lval[1])zpush();
	rvalue(lval);
	inc();
	ptr=lval[0];
	if((ptr[ident]==pointer)&
		(ptr[type]==cint))
			inc();
	store(lval);
}
heir10dec(lval)
	int lval[];
{
	char *ptr;


	if(lval[1])zpush();
	rvalue(lval);
	dec();
	ptr=lval[0];
	if((ptr[ident]==pointer)&
		(ptr[type]==cint))
			dec();
	store(lval);
}
heir10as(lval)
	int lval[];
{
	int k;
	char *ptr;


	k=heir10(lval);
	if(k)rvalue(lval);
	lval[1]=cint;
	if(ptr=lval[0])lval[1]=ptr[type];
	lval[0]=0;
}
/* ### cpcn-35 */
heir10at(lval)
	int lval[];
{
	char *ptr;
	getloc(ptr=lval[0]);
	lval[1]=ptr[type];
}
heir10id(lval)
	int lval[];
{
	char *ptr;


	if(lval[1])zpush();
	rvalue(lval);
	inc();
	ptr=lval[0];
	if((ptr[ident]==pointer)&
		(ptr[type]==cint))
			inc();
	store(lval);
	dec();
	if((ptr[ident]==pointer)&
		(ptr[type]==cint))
		dec();
}
heir10di(lval)
	int lval[];
{
	char *ptr;


	if(lval[1])zpush();
	rvalue(lval);
	dec();
	ptr=lval[0];
	if((ptr[ident]==pointer)&
		(ptr[type]==cint))
			dec();
	store(lval);
	inc();
	if((ptr[ident]==pointer)&
		(ptr[type]==cint))
		inc();
}
/* ### cpcn-36 */
/*	>>>>>> start of cc7 <<<<<<	*/


heir11(lval)
	int *lval;
{	int k;char *ptr;
	k=primary(lval);
	ptr=lval[0];
	blanks();
	if((ch()=='[')|(ch()=='('))
	while(1)
		{if(match("["))
			{if(ptr==0)
				{error("can't subscript");
				junk();
				needbrack("]");
				return 0;
				}
			else if(ptr[ident]==pointer)rvalue(lval);
			else if(ptr[ident]!=array)
				{error("can't subscript");
				k=0;
				}
			zpush();
			expression();
			needbrack("]");
			if(ptr[type]==cint)doublereg();
			zpop();
			zadd();
			lval[1]=ptr[type];
			k=1;
			}
		else if(match("("))
			{if(ptr==0)
				{callfunction(0);
				}
			else if(ptr[ident]!=function)
				{rvalue(lval);
				callfunction(0);
				}
			else callfunction(ptr);
			k=lval[0]=0;
			}
		else return k;
		}
	if (ptr) {
		if (ptr[ident]==function) {
			getloc(ptr);
			return 0;
		}
	}
	return k;
}
primary(lval)
	int *lval;
{	char *ptr,sname[namesize];int num[1];
	int k;
	if (match("(")) {
		k=heir1(lval);
		needbrack(")");
		return k;
	}
	else if (symname(sname)) {
		ptr=findloc(sname);
		if (ptr==0) {
			ptr=findglb(sname);
			if (ptr==0) {
				ptr=addglb(sname,function,cint,0);
			}
		}
		lval[0]=ptr;
		lval[1]=0;
		if (ptr[ident]==function) {
			return 0;
		}
		else if (ptr[ident]==array) {
			getloc(ptr);
			lval[1]=ptr[type];
			return 0;
		}
		else {
			return 1;
		}
	}
	else if (constant(num)) {
		return lval[0]=lval[1]=0;
	}
	else {
		error("invalid expression");
		junk();
		return 0;
	}
}
/* ### cpcn-38 */
store(lval)
	int *lval;
{	if (lval[1]==0)putmem(lval[0]);
	else putstk(lval[1]);
}
rvalue(lval)
	int *lval;
{	if((lval[0] != 0) & (lval[1] == 0))
		getmem(lval[0]);
		else indirect(lval[1]);
}
test(label)
	int label;
{
	needbrack("(");
	expression();
	needbrack(")");
	testjump(label);
}
constant(val)
	int val[];
{	if (number(val))
		immed();
	else if (pstr(val))
		immed();
	else if (qstr(val))
		{immed();outstr("OFFSET ");printlabel(litlab);outbyte('+');}
	else return 0;	
	outdec(val[0]);
	nl();
	return 1;
}
/* ### cpcn-39 */
number(val)
	int val[];
{	int k,minus;
	k=minus=1;
	while(k)
		{k=0;
		if (match("+")) k=1;
		if (match("-")) {minus=(-minus);k=1;}
		}
	if(numeric(ch())==0)return 0;
	while (numeric(ch()))
		k=k*10+(gch()-'0');
	if (minus<0) k=(-k);
	val[0]=k;
	return 1;
}
pstr(val)
	int val[];
{	int c,k;
	k=0;
	if (match("'")==0) return 0;
	while((c=gch())!=39)
		k=c;
	val[0]=k;
	return 1;
}
qstr(val)
	int val[];
{
	if (match(quote)==0) return 0;
	val[0]=litptr;
	while (ch()!='"')
		{if(ch()==0)break;
		if(litptr>=litmax)
			{error("string space exhausted");
			while(match(quote)==0)
				if(gch()==0)break;
			return 1;
			}
		litq[litptr++]=gch();
		}
	gch();
	litq[litptr++]=0;
	return 1;
}
/* ### cpcn-40 */
/*	>>>>>> start of cc8 <<<<<<<	*/


/* Begin a comment line for the assembler */
comment()
{	outbyte(';');
}


/* Put out assembler info before any code is generated */
header()
{
	outstr("CSEG SEGMENT BYTE PUBLIC 'code'");nl();
	ol("ASSUME CS:CSEG,SS:STACK");
}
/* Print any assembler stuff needed after all code */
trailer()
{
	ol("END");
}
/* ### cpcn-41 */
/* Print out a name such that it won't annoy the assembler */
outname(sname)
	char *sname;
{
	outasm("qz");
	outasm(sname);
}
/* Fetch a static memory cell into the primary register */
getmem(sym)
	char *sym;
{
	if (sym[storage]==stkloc) {
		ol("MOV BP,SP");
	}
	if ((sym[ident]!=pointer)&(sym[type]==cchar)) {
		ot("MOV AL,");
	}
	else {
		ot("MOV BX,");
	}
	if (sym[storage]==stkloc) {
		outdec((sym[offset]&255)+(sym[offset+1]<<8)-Zsp);
		outstr("[BP]");
	}
	else {
		outstr("SS:");
		outname(sym+name);
	}
	nl();
	if ((sym[ident]!=pointer)&(sym[type]==cchar)) {
		ol("CBW");
		ol("MOV BX,AX");
	}
}
/* Fetch the address of the specified symbol */
/*	into the primary register */
getloc(sym)
	char *sym;
{
	int off;
	if (sym[storage]==stkloc) {
		ol("MOV BX,SP");
		off=(sym[offset]&255)+(sym[offset+1]<<8)-Zsp;
		if (off) {
			ot("ADD BX,");
			outdec(off);
			nl();
		}
	}
	else {
		ot("MOV BX,OFFSET ");
		outname(sym+name);
		nl();
	}
}
/* Store the primary register into the specified */
/*	static memory cell */
putmem(sym)
	char *sym;
{
	if (sym[storage]==stkloc) {
		ol("MOV BP,SP");
		ot("MOV ");
		outdec((sym[offset]&255)+(sym[offset+1]<<8)-Zsp);
		outstr("[BP]");
	}
	else {
		ot("MOV SS:");outname(sym+name);
	}
	if ((sym[ident]!=pointer)&(sym[type]==cchar)) {
		outstr(",BL");
	}
	else {
		outstr(",BX");
	}
	nl();
}
/* Store the specified object type in the primary register */
/*	at the address on the top of the stack */
putstk(typeobj)
	char typeobj;
{
	zpop();
	ol("MOV BP,DX");
	if (typeobj==cchar) {
		ol("MOV [BP],BL");
	}
	else {
		ol("MOV [BP],BX");
	}
}
/* ### cpcn-42 */
/* Fetch the specified object type indirect through the */
/*	primary register into the primary register */
indirect(typeobj)
	char typeobj;
{
	if (typeobj==cchar) {
		ol("MOV AL,SS:[BX]");
		ol("CBW");
		ol("MOV BX,AX");
	}
	else {
		ol("MOV BX,SS:[BX]");
	}
}
/* Swap the primary and secondary registers */
swap()
{
	ol("XCHG DX,BX");
}
/* Print partial instruction to get an immediate value */
/*	into the primary register */
immed()
{
	ot("MOV BX,");
}
/* Push the primary register onto the stack */
zpush()
{
	ol("PUSH BX");
	Zsp=Zsp-2;
}
/* Pop the top of the stack into the secondary register */
zpop()
{
	ol("POP DX");
	Zsp=Zsp+2;
}
/* Swap the primary register and the top of the stack */
swapstk()
{
	ol("MOV BP,SP");
	ol("XCHG BX,[BP]");
}
/* Call the specified subroutine name */
zcall(sname)
	char *sname;
{	ot("CALL ");
	outname(sname);
	nl();
	addext(sname,userfunc);
}

/* Return from subroutine */
zret()
{	ol("RET");
}
/* ### cpcn-43 */
/* Perform subroutine call to value on top of stack */
callstk()
{	immed();
	outasm("$+11");
	nl();
	swapstk();
	ol("JMP BX");
	Zsp=Zsp-2;
	}
/* Jump to specified internal label number */
jump(label)
	int label;
{	ot("JMP ");
	printlabel(label);
	nl();
	}
/* Test the primary register and jump if false to label */
testjump(label)
	int label;
{	
	ol("OR BX,BX");
	ol("JNZ $+5");
	jump(label);
}
/* Print pseudo-op to define a byte */
defbyte()
{	ot("DB ");
}
/* Print pseudo-op to define a word */
defword()
{	ot("DW ");
}
/* Modify the stack pointer to the new value indicated */
modstk(newsp)
	int newsp;
{	int k;
	k=newsp-Zsp;
	if (k) {
		if (k==1) {
			ol("INC SP");
		}
		else if (k==-1) {
			ol("DEC SP");
		}
		else {
			ot("ADD SP,");
			outdec(k);nl();
		}
	}
	return newsp;
}
/* ### cpcn-44 */
/* Double the primary register */
doublereg()
{	ol("ADD BX,BX");
}
/* Add the primary and secondary registers */
/*	(results in primary) */
zadd()
{	ol("ADD BX,DX");
}
/* Subtract the primary register from the secondary */
/*	(results in primary) */
zsub()
{
	ol("SUB BX,DX");
	ol("NEG BX");
}
/* Multiply the primary and secondary registers */
/*	(results in primary */
mult()
{
	ol("MOV AX,DX");
	ol("IMUL BX");
	ol("MOV BX,AX");
}
/* Divide the secondary register by the primary */
/*	(quotient in primary, remainder in secondary) */
div()
{
	ol("MOV AX,DX");
	ol("CWD");
	ol("IDIV BX");
	ol("MOV BX,AX");
}
/* Compute remainder (mod) of secondary register divided */
/*	by the primary */
/*	(remainder in primary, quotient in secondary) */
zmod()
{
	ol("MOV AX,DX");
	ol("CWD");
	ol("IDIV BX");
	ol("MOV BX,DX");
}
/* Inclusive 'or' the primary and the secondary registers */
/*	(results in primary) */
zor()
	{ol("OR BX,DX");}
/* Exclusive 'or' the primary and seconday registers */
/*	(results in primary) */
zxor()
	{ol("XOR BX,DX");}
/* 'And' the primary and secondary registers */
/*	(results in primary) */
zand()
	{ol("AND BX,DX");}
/* Arithmetic shift right the secondary register number of */
/* 	times in primary (results in primary) */
asr()
	{ol("MOV CL,BL");ol("MOV BX,DX");ol("SAR BX,CL");}
/* Arithmetic left shift the secondary register number of */
/*	times in primary (results in primary) */
asl()
	{ol("MOV CL,BL");ol("MOV BX,DX");ol("SAL BX,CL");}
/* Form two's complement of primary register */
neg()
	{ol("NEG BX");}
/* Form one's complement of primary register */
com()
	{ol("NOT BX");}
/* Increment the primary register by one */
inc()
	{ol("INC BX");}
/* Decrement the primary register by one */
dec()
	{ol("DEC BX");}


/* Following are the conditional operators */
/* They compare the secondary register against the primary */
/* and put a literal 1 in the primary if the condition is */
/* true, otherwise they clear the primary register */


/* Test for equal */
zeq()
{
	ol("CMP DX,BX");
	ol("MOV BX,1");
	ol("JE $+3");
	ol("DEC BX");
}
/* Test for not equal */
zne()
{
	ol("CMP DX,BX");
	ol("MOV BX,1");
	ol("JNE $+3");
	ol("DEC BX");
}
/* Test for less than (signed) */
zlt()
{
	ol("CMP DX,BX");
	ol("MOV BX,1");
	ol("JL $+3");
	ol("DEC BX");
}
/* Test for less than or equal to (signed) */
zle()
{
	ol("CMP DX,BX");
	ol("MOV BX,1");
	ol("JLE $+3");
	ol("DEC BX");
}
/* Test for greater than (signed) */
zgt()
{
	ol("CMP DX,BX");
	ol("MOV BX,1");
	ol("JG $+3");
	ol("DEC BX");
}
/* Test for greater than or equal to (signed) */
zge()
{
	ol("CMP DX,BX");
	ol("MOV BX,1");
	ol("JGE $+3");
	ol("DEC BX");
}
/* Test for less than (unsigned) */
ult()
{
	ol("CMP DX,BX");
	ol("MOV BX,1");
	ol("JB $+3");
	ol("DEC BX");
}
/* Test for less than or equal to (unsigned) */
ule()
{
	ol("CMP DX,BX");
	ol("MOV BX,1");
	ol("JBE $+3");
	ol("DEC BX");
}
/* Test for greater than (unsigned) */
ugt()
{
	ol("CMP DX,BX");
	ol("MOV BX,1");
	ol("JA $+3");
	ol("DEC BX");
}
/* Test for greater than or equal to (unsigned) */
uge()
{
	ol("CMP DX,BX");
	ol("MOV BX,1");
	ol("JAE $+3");
	ol("DEC BX");
}

