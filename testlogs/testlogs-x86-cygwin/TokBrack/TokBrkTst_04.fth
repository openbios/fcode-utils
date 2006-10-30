\  Test of long names and duplication of names and maybe Tracing 
\   Updated Wed, 18 Oct 2006 at 13:34 PDT by David L. Paktor

fcode-version2

global-definitions

true constant flunky?

alias whoosis whatsis
alias whatsis whoosis

external
decimal

100 constant pele_yoetz_ne-ema-an_anochi_hu_ha-omer_v-oseh_v-ain_c-moni_bchol_ha_aretz

device-definitions

headers

h#  3760  constant whatsis

#message" Sync Up Diffs w/ prev. release."n"
alias whoosis pele_yoetz_ne-ema-an_anochi_hu_ha-omer_v-oseh_v-ain_c-moni_bchol_ha_aretz
#message"
Sync Up again."n"
headerless
: pele_yoetz_ne-ema-an_anochi_hu_ha-omer_v-oseh_v-ain_c-moni_bchol_ha_aretz
     100
;
pele_yoetz_ne-ema-an_anochi_hu_ha-omer_v-oseh_v-ain_c-moni_bchol_ha_aretz 2 * constant clone

\  Make sure the matching goes all the way...
instance
: pele_yoetz_ne-ema-an_anochi_hu_ha-omer_v-oseh_v-ain_c-moni_bchol_ha_oilum
     clone
     ['] whoosis execute
;

new-device
80 constant  this_name_has_a_whole_lot_of_syllables_and_so_would_not_be_a_good_ingredient_in_ice_cream_but_at_least_you_know_what_it_means

headers
  f[    1cec6ea3  constant  a_name_with_too_many_letters_should_not_matter_in_tokenizer_escape_mode
   ]f

: brand-x-ice-cream  \  Because it uses ingredients with too many syllables
    pele_yoetz_ne-ema-an_anochi_hu_ha-omer_v-oseh_v-ain_c-moni_bchol_ha_aretz    dup 20 - do  i . loop  cr
   ." Nameless one..." this_name_has_a_whole_lot_of_syllables_and_so_would_not_be_a_good_ingredient_in_ice_cream_but_at_least_you_know_what_it_means  ." equals " . cr  
   ." But everyone likes an "
   f[   a_name_with_too_many_letters_should_not_matter_in_tokenizer_escape_mode
   f]      fliteral  .
    clone . cr
;

flunky? if
    d# 3760 constant whatsis
else
    d#  374 constant whatsis
then


finish-device

overload : clone
    pele_yoetz_ne-ema-an_anochi_hu_ha-omer_v-oseh_v-ain_c-moni_bchol_ha_aretz
    pele_yoetz_ne-ema-an_anochi_hu_ha-omer_v-oseh_v-ain_c-moni_bchol_ha_oilum
    whatsis
    whoosis
;

end0



