
## $Id$

## Makefile.am -- Process this file with automake to produce Makefile.in

bin_PROGRAMS = smart icp

## Smart

smart_SOURCES = smart.cc 

smart_LDADD = \
  ../Engines/simul.la ../Engines/exact.la \
  ../Engines/ctl_exp.la ../Engines/ctl_symb.la ../Engines/csl_exp.la \
  ../Engines/gen_rg_base.la \
  ../Engines/gen_exp_as.la ../Engines/gen_exp_ph.la \
  ../Engines/proc_meddly.la \
  ../Engines/satur_meddly.la ../Engines/expl_meddly.la \
  \
  ../Formlsms/mc_form.la ../Formlsms/fsm_form.la \
  ../Formlsms/pn_form.la ../Formlsms/evm_form.la ../Formlsms/tam_form.la \
  ../Formlsms/basic_msr.la ../Formlsms/ctl_msr.la \
  ../Formlsms/stoch_msr.la ../Formlsms/csl_msr.la \
  ../Formlsms/enum_hlm.la ../Formlsms/dsde_hlm.la ../Formlsms/phase_hlm.la \
  ../Formlsms/state_llm.la ../Formlsms/graph_llm.la ../Formlsms/stoch_llm.la \
  ../Formlsms/mc_llm.la ../Formlsms/fsm_llm.la \
  ../Formlsms/rss_mdd.la ../Formlsms/fsm_mdd.la ../Formlsms/mc_mdd.la \
  ../Formlsms/rss_indx.la ../Formlsms/rss_enum.la ../Formlsms/rss_expl.la \
  \
  ../Modules/stringtype.la ../Modules/biginttype.la \
  ../Modules/stochtypes.la ../Modules/glue_meddly.la \
  ../Modules/expl_states.la \
  ../Modules/statesets.la ../Modules/expl_ssets.la ../Modules/meddly_ssets.la \
  ../Modules/statevects.la \
  \
  ../ParseSM/libsmparse.la \
  $(TIMER_LIBRARY)/lib/libtimer.a \
  $(LS_LIBRARY)/lib/libls.a \
  $(MC_LIBRARY)/lib/libmc.a \
  $(GRAPH_LIBRARY)/lib/libgraph.a \
  $(STATE_LIBRARY)/lib/libstate.a \
  $(IS_LIBRARY)/lib/libintset.a \
  $(SIM_LIBRARY)/lib/libsim.a \
  $(RNG_LIBRARY)/lib/librng.a \
  ../SymTabs/libst.la \
  ../FuncLib/libfuncs.la \
  ../ExprLib/libexpr.la \
  ../Options/libopts.la \
  ../Streams/libstream.la \
  $(MDD_LIBRARY)/lib/libmeddly.a

## ICP   

icp_SOURCES = icp.cc

icp_LDADD = \
  ../Formlsms/dcp_msr.la ../Formlsms/dcp_form.la ../Formlsms/noevnt_hlm.la \
  ../Engines/dcp_expl.la ../Engines/dcp_symb.la \
  ../Modules/glue_meddly.la ../Modules/biginttype.la \
  ../ParseICP/libicpparse.la \
  $(TIMER_LIBRARY)/lib/libtimer.a \
  $(STATE_LIBRARY)/lib/libstate.a \
  ../SymTabs/libst.la \
  ../ExprLib/libexpr.la \
  ../Options/libopts.la \
  ../Streams/libstream.la \
  $(MDD_LIBRARY)/lib/libmeddly.a

