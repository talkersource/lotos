/*****************************************************************************
                    Struktura pre makra v OS Star v1.1.1
            Copyright (C) Pavol Hluchy - posledny update: 5.10.2000
          osstar@star.sjf.stuba.sk  |  http://star.sjf.stuba.sk/osstar
 *****************************************************************************/

struct macro_struct {
	char name[MC_NAME_LEN+1], comstr[MC_COM_LEN+1];
	struct macro_struct *prev, *next;
	};
typedef struct macro_struct *MC_OBJECT;