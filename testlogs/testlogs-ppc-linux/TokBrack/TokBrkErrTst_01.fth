\  Test of  tokenizer[    ]tokenizer  scope Error-Detection 
\   Updated Fri, 17 Feb 2006 at 10:15 PST by David L. Paktor

fcode-version2

headers
  h# 30
    emit-byte
hex
tokenizer[
           rummidge
	 10 emit-byte
           10 emit-byte
	     d# 10 emit-byte
	      o# 10 emit-byte
	        h# 10 emit-byte
	   802 next-fcode
	   h# 1020 next-fcode
	   h# 3682 emit-byte
	 ]tokenizer
decimal 50 constant gummidge
hex
tokenizer[  gummidge
           rummidge
	 ]tokenizer
  h# 30 emit-byte

100 constant pele_yoetz_ne'ema'an_anochi_hu_ha'omer_v'oseh_v'ain_c'moni_bchol_ha_aretz

pele_yoetz_ne'ema'an_anochi_hu_ha'omer_v'oseh_v'ain_c'moni_bchol_ha_aretz 2 * constant clone

b(lit)
tokenizer[  
    30 swap  emit-byte
	     d# 20 emit-byte
	      o# 20 emit-byte
	        h# 20 emit-byte
	 ]tokenizer
tokenizer[ 
h# defeca8e  constant poopoo
h# beeffece  constant moopoo
	 ]tokenizer
     alias merde poopoo
     alias couterde moopoo
tokenizer[ 
     merde fliteral
	 ]tokenizer
     couterde fliteral

tokenizer[ 
    fcode-push
    fliteral
    a# Fink
    fcode-pop
	 ]tokenizer

end0



