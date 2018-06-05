
## Makefile.am -- Process this file with automake to produce Makefile.in

# For serial-tests
# TESTS_ENVIRONMENT = $(srcdir)/TestProg

TESTS =   arrays.sm

if LINK_GMP
TESTS +=  bigints.sm
endif

TESTS +=  converge.sm courier.sm ctmc_csl.sm ctmc_ctl.sm ctmc_tran.sm \
          cvgcvg.sm cvgfor.sm distrange.sm dontknow.sm \
          dox_cast.sm dox_ops.sm \
          dtmc_csl.sm dtmc_ctl.sm dtmc_tran.sm dtmcs.sm \
          fms.sm forcvg.sm forloops.sm fsm_ctl.sm fsms.sm funcs.sm \
          include.sm inputs.sm \
          kanban.sm \
          lexical.sm \
          math.sm mc_classes.sm mdrate.sm models.sm \
          partition.sm \
          phase_add.sm phase_choose.sm phase_cmult.sm phase_dmult.sm phase_order.sm \
          phase_distros.sm phase_tta.sm phase_TF.sm phase_TU.sm \
          phase_deps.sm phase_avg.sm phase_var.sm \
          pn_assert.sm pn_ctl_expl.sm 
          
TESTS +=  pn_ctl_meddly.sm 

TESTS +=  pns.sm print.sm priority.sm \
          queens-expl.sm \
          rg_hash.sm rg_rb.sm rg_splay.sm 

TESTS +=  rg_mddexpl.sm rg_sat_act.sm rg_sat_pot.sm 

TESTS +=  sim_avg.sm strings.sm \
          ttt_evm.sm types.sm \
          vanish.sm \
          \
          bridge3.icp bridge4.icp bridge5.icp kakuro_1.icp \
          queens_06.icp queens_08.icp queens_10.icp queens_12.icp \
          sudoku_1.icp sudoku_2.icp sudoku_3.icp

TEST_EXTENSIONS = .sm .icp

SM_LOG_DRIVER = $(srcdir)/TestSM
ICP_LOG_DRIVER = $(srcdir)/TestICP

check-old:
	$(srcdir)/TestAny ../src $(srcdir) $(TESTS)

bench:
	$(srcdir)/Benchmark ../src $(srcdir) $(TESTS)
