\  Elementary test of the  tokenizer[    ]tokenizer  scope functions 
\   Updated Fri, 21 Oct 2005 at 16:01 PDT by David L. Paktor

fcode-version2

headers
hex

b(lit)
tokenizer[
           10 emit-byte
	     d# 10 emit-byte
	      o# 10 emit-byte
	        h# 10 emit-byte
	 ]tokenizer
decimal
100 constant pele_yoetz_ne'ema'an_anochi_hu_ha'omer_v'oseh_v'ain_c'moni_bchol_ha_aretz

pele_yoetz_ne'ema'an_anochi_hu_ha'omer_v'oseh_v'ain_c'moni_bchol_ha_aretz 2 * constant clone

12 c,
tokenizer[  12  fliteral
	 ]tokenizer  l,
12 c,
: yuttzer  14  tokenizer[  14 fliteral  hex 14 fliteral ]tokenizer 14 ;

hex
headerless
80 constant  this_name_has_a_whole_lot_of_syllables_and_so_would_not_be_a_good_ingredient_in_ice_cream_but_at_least_you_know_what_it_means

headers

  f[    1cec6ea3  constant  a_name_with_too_many_letters_should_not_matter_in_tokenizer_escape_mode
   ]f

: brand-x-ice-cream  \  Because it uses ingredients with too many syllables
    pele_yoetz_ne'ema'an_anochi_hu_ha'omer_v'oseh_v'ain_c'moni_bchol_ha_aretz
    dup 20 - do  i . loop  cr
   ." Nameless one..." this_name_has_a_whole_lot_of_syllables_and_so_would_not_be_a_good_ingredient_in_ice_cream_but_at_least_you_know_what_it_means  ." equals " . cr  
   ." But everyone likes an "
   f[   a_name_with_too_many_letters_should_not_matter_in_tokenizer_escape_mode
   f]      fliteral  .
    clone . cr
;

end0



