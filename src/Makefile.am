
## Makefile.am -- Process this file with automake to produce Makefile.in

## List any subdirectories where we might want/need to run make
SUBDIRS = _LSLib _RngLib _SimLib _IntSets _StateLib _GraphLib _MCLib _Meddly


## List executables that we build
bin_PROGRAMS = smart icp teststrms testsplay testinit


BUILT_SOURCES = \
  ParseSM/smartlex.cc ParseSM/smartyacc.hh ParseSM/smartyacc.cc \
  ParseICP/icplex.cc ParseICP/icpyacc.hh ParseICP/icpyacc.cc 

AM_YFLAGS = -d
AM_CXXFLAGS = -Wall

smart_LDFLAGS=${LDFLAGS} -all-static
smart_CXXFLAGS =${CXXFLAGS} -Wall -O3 -DNDEBUG

## ============================================================
##
## SMART
##
## ============================================================

smart_SOURCES = \
  _Timer/timerlib.cc \
  \
  _LSLib/lslib.cc \
  \
  _RngLib/rng.cc \
  \
  _SimLib/sim.cc _SimLib/normal.cc _SimLib/autocorrelation.cc \
  \
  _IntSets/intset.cc \
  \
  _StateLib/statelib.cc _StateLib/coll.cc _StateLib/bst_db.cc \
  _StateLib/splaydb.cc _StateLib/rb_db.cc _StateLib/hash_db.cc \
  \
  _GraphLib/graphlib.cc _GraphLib/sccs.cc \
  \
  _Distros/distros.cc \
  \
  _MCLib/mclib.cc _MCLib/error.cc _MCLib/vanishing.cc \
  _MCLib/markov_chain.cc \
  \
  Streams/streams.cc  Streams/textfmt.cc \
  \
  Options/options.cc \
  \
  ExprLib/startup.cc \
  ExprLib/exprman.cc \
  ExprLib/superman.cc \
  ExprLib/engine.cc \
  ExprLib/formalism.cc \
  ExprLib/init_data.cc \
  ExprLib/type.cc \
  ExprLib/result.cc \
  ExprLib/expr.cc \
  ExprLib/bogus.cc \
  ExprLib/values.cc \
  ExprLib/symbols.cc \
  ExprLib/iterators.cc \
  ExprLib/arrays.cc \
  ExprLib/stmts.cc \
  ExprLib/forloops.cc \
  ExprLib/functions.cc \
  ExprLib/converge.cc \
  ExprLib/sets.cc \
  ExprLib/strings.cc \
  ExprLib/intervals.cc \
  ExprLib/unary.cc \
  ExprLib/casting.cc \
  ExprLib/binary.cc \
  ExprLib/trinary.cc \
  ExprLib/assoc.cc \
  ExprLib/ops_bool.cc \
  ExprLib/ops_int.cc \
  ExprLib/ops_real.cc \
  ExprLib/ops_set.cc \
  ExprLib/ops_misc.cc \
  ExprLib/dd_front.cc \
  ExprLib/mod_def.cc \
  ExprLib/mod_vars.cc \
  ExprLib/mod_inst.cc \
  ExprLib/measures.cc \
  \
  SymTabs/symtabs.cc \
  \
  Modules/biginttype.cc \
  Modules/expl_ssets.cc \
  Modules/expl_states.cc \
  Modules/glue_meddly.cc \
  Modules/meddly_ssets.cc \
  Modules/statesets.cc \
  Modules/statevects.cc \
  Modules/stochtypes.cc \
  Modules/stringtype.cc \
  Modules/temporal.cc \
  Modules/trace.cc \
  \
  Formlsms/basic_msr.cc \
  Formlsms/csl_msr.cc \
  Formlsms/ctl_msr.cc \
  Formlsms/dsde_hlm.cc \
  Formlsms/enum_hlm.cc \
  Formlsms/evm_form.cc \
  Formlsms/fsm_form.cc \
  Formlsms/graph_llm.cc \
  Formlsms/mc_form.cc \
  Formlsms/noevnt_hlm.cc \
  Formlsms/phase_hlm.cc \
  Formlsms/pn_form.cc \
  Formlsms/proc_markov.cc \
  Formlsms/proc_mclib.cc \
  Formlsms/proc_meddly.cc \
  Formlsms/rgr_ectl.cc \
  Formlsms/rgr_grlib.cc \
  Formlsms/rgr_meddly.cc \
  Formlsms/rss_enum.cc \
  Formlsms/rss_expl.cc \
  Formlsms/rss_indx.cc \
  Formlsms/rss_meddly.cc \
  Formlsms/show_graph.cc \
  Formlsms/state_llm.cc \
  Formlsms/stoch_llm.cc \
  Formlsms/stoch_msr.cc \
  Formlsms/tam_form.cc \
  \
  Engines/csl_exp.cc \
  Engines/exact.cc \
  Engines/expl_meddly.cc \
  Engines/gen_exp_as.cc \
  Engines/gen_exp_ph.cc \
  Engines/gen_meddly.cc \
  Engines/gen_rg_base.cc \
  Engines/satur_meddly.cc \
  Engines/simul.cc \
  Engines/order_base.cc \
  \
  FuncLib/basefuncs.cc \
  FuncLib/funclib.cc \
  FuncLib/iofuncs.cc \
  FuncLib/mathfuncs.cc \
  FuncLib/sysfuncs.cc \
  FuncLib/topics.cc \
  \
  ParseSM/smartlex.ll \
  ParseSM/smartyacc.yy \
  ParseSM/lexer.cc \
  ParseSM/compile.cc \
  ParseSM/parse_sm.cc \
  \
  Apps/smart.cc


smart_LDADD = _Meddly/src/.libs/libmeddly.a


## ============================================================
##
## ICP
##
## ============================================================

icp_SOURCES = \
  _Timer/timerlib.cc \
  \
  _StateLib/statelib.cc _StateLib/coll.cc _StateLib/bst_db.cc \
  _StateLib/splaydb.cc _StateLib/rb_db.cc _StateLib/hash_db.cc \
  \
  Streams/streams.cc  Streams/textfmt.cc \
  \
  Options/options.cc \
  \
  ExprLib/startup.cc \
  ExprLib/exprman.cc \
  ExprLib/superman.cc \
  ExprLib/engine.cc \
  ExprLib/formalism.cc \
  ExprLib/init_data.cc \
  ExprLib/type.cc \
  ExprLib/result.cc \
  ExprLib/expr.cc \
  ExprLib/bogus.cc \
  ExprLib/values.cc \
  ExprLib/symbols.cc \
  ExprLib/iterators.cc \
  ExprLib/arrays.cc \
  ExprLib/stmts.cc \
  ExprLib/forloops.cc \
  ExprLib/functions.cc \
  ExprLib/converge.cc \
  ExprLib/sets.cc \
  ExprLib/strings.cc \
  ExprLib/intervals.cc \
  ExprLib/unary.cc \
  ExprLib/casting.cc \
  ExprLib/binary.cc \
  ExprLib/trinary.cc \
  ExprLib/assoc.cc \
  ExprLib/ops_bool.cc \
  ExprLib/ops_int.cc \
  ExprLib/ops_real.cc \
  ExprLib/ops_set.cc \
  ExprLib/ops_misc.cc \
  ExprLib/dd_front.cc \
  ExprLib/mod_def.cc \
  ExprLib/mod_vars.cc \
  ExprLib/mod_inst.cc \
  ExprLib/measures.cc \
  \
  SymTabs/symtabs.cc \
  \
  Modules/biginttype.cc \
  Modules/glue_meddly.cc \
  \
  Formlsms/dcp_msr.cc \
  Formlsms/dcp_form.cc \
  Formlsms/noevnt_hlm.cc \
  \
  Engines/dcp_expl.cc \
  Engines/dcp_symb.cc \
  \
  ParseICP/icplex.ll \
  ParseICP/icpyacc.yy \
  ParseICP/lexer.cc \
  ParseICP/compile.cc \
  ParseICP/parse_icp.cc \
  \
  Apps/icp.cc


icp_LDADD = _Meddly/src/libmeddly.la


## ============================================================
##
## teststrms
##
## ============================================================

teststrms_SOURCES = \
  TestApps/teststrms.cc \
  Streams/streams.cc  Streams/textfmt.cc 

## ============================================================
##
## testsplay
##
## ============================================================

testsplay_SOURCES = \
  TestApps/testsplay.cc \
  Streams/streams.cc  Streams/textfmt.cc 

## ============================================================
##
## testinit 
##
## ============================================================

testinit_SOURCES = \
  TestApps/testinit.cc TestApps/initd.cc \
  ExprLib/startup.cc
