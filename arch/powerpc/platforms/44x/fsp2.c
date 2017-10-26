/*
 * FSP-2 board specific routines
 *
 * Based on earlier code:
 *    Matt Porter <mporter@kernel.crashing.org>
 *    Copyright 2002-2005 MontaVista Software Inc.
 *
 *    Eugene Surovegin <eugene.surovegin@zultys.com> or <ebs@ebshome.net>
 *    Copyright (c) 2003-2005 Zultys Technologies
 *
 *    Rewritten and ported to the merged powerpc tree:
 *    Copyright 2007 David Gibson <dwg@au1.ibm.com>, IBM Corporation.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/init.h>
#include <linux/of_platform.h>
#include <linux/rtc.h>

#include <asm/machdep.h>
#include <asm/prom.h>
#include <asm/udbg.h>
#include <asm/time.h>
#include <asm/uic.h>
#include <asm/ppc4xx.h>
#include <asm/fsp2_reg.h>
#include <asm/dcr.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>

#define FSP2_BUS_ERR	"ibm,bus-error-irq"
#define FSP2_CMU_ERR	"ibm,cmu-error-irq"
#define FSP2_CONF_ERR	"ibm,conf-error-irq"
#define FSP2_OPBD_ERR	"ibm,opbd-error-irq"
#define FSP2_MCUE	"ibm,mc-ue-irq"
#define FSP2_RST_WRN	"ibm,reset-warning-irq"

static __initdata struct of_device_id fsp2_of_bus[] = {
	{ .compatible = "ibm,plb4", },
	{ .compatible = "ibm,plb6", },
	{ .compatible = "ibm,opb", },
	{},
};

static u32 mfcmu(u32 reg)
{
	u32 data;

	mtdcr(DCRN_CMU_ADDR, reg);
	data = mfdcr(DCRN_CMU_DATA);
	return data;
}

static void mtcmu(u32 reg, u32 data)
{
	mtdcr(DCRN_CMU_ADDR, reg);
	mtdcr(DCRN_CMU_DATA, data);
}

static void mtl2(u32 reg, u32 data)
{
	mtdcr(DCRN_L2CDCRAI, reg);
	mtdcr(DCRN_L2CDCRDI, data);
}

static u32 mfl2(u32 reg) {
	u32 data;

	mtdcr(DCRN_L2CDCRAI, reg);
	data = mfdcr(DCRN_L2CDCRDI);
	return data;
}

static void l2regs(void) {
	printk("L2 Controller:\n");
	printk("MCK:      0x%08x\n", mfl2(L2MCK));
	printk("INT:      0x%08x\n", mfl2(L2INT));
	printk("PLBSTAT0: 0x%08x\n", mfl2(L2PLBSTAT0));
	printk("PLBSTAT1: 0x%08x\n", mfl2(L2PLBSTAT1));
	printk("ARRSTAT0: 0x%08x\n", mfl2(L2ARRSTAT0));
	printk("ARRSTAT1: 0x%08x\n", mfl2(L2ARRSTAT1));
	printk("ARRSTAT2: 0x%08x\n", mfl2(L2ARRSTAT2));
	printk("CPUSTAT:  0x%08x\n", mfl2(L2CPUSTAT));
	printk("RACSTAT0: 0x%08x\n", mfl2(L2RACSTAT0));
	printk("WACSTAT0: 0x%08x\n", mfl2(L2WACSTAT0));
	printk("WACSTAT1: 0x%08x\n", mfl2(L2WACSTAT1));
	printk("WACSTAT2: 0x%08x\n", mfl2(L2WACSTAT2));
	printk("WDFSTAT:  0x%08x\n", mfl2(L2WDFSTAT));
	printk("LOG0:     0x%08x\n", mfl2(L2LOG0));
	printk("LOG1:     0x%08x\n", mfl2(L2LOG1));
	printk("LOG2:     0x%08x\n", mfl2(L2LOG2));
	printk("LOG3:     0x%08x\n", mfl2(L2LOG3));
	printk("LOG4:     0x%08x\n", mfl2(L2LOG4));
	printk("LOG5:     0x%08x\n", mfl2(L2LOG5));
}

static void show_plbopb_regs(u32 base, int num)
{
	printk("\nPLBOPB Bridge %d:\n", num);
	printk("GESR0: 0x%08x\n", mfdcr(base + PLB4OPB_GESR0));
	printk("GESR1: 0x%08x\n", mfdcr(base + PLB4OPB_GESR1));
	printk("GESR2: 0x%08x\n", mfdcr(base + PLB4OPB_GESR2));
	printk("GEARU: 0x%08x\n", mfdcr(base + PLB4OPB_GEARU));
	printk("GEAR:  0x%08x\n", mfdcr(base + PLB4OPB_GEAR));
}

static irqreturn_t error_irq_handler(int irq, void *data)
{
	u32 crcs;
	struct device_node *np = data;

	if (of_device_is_compatible(np, FSP2_BUS_ERR)) {
		printk("Bus Error\n");

		l2regs();

		printk("\nPLB6 Controller:\n");
		printk("BC_SHD: 0x%08x\n", mfdcr(DCRN_PLB6_SHD));
		printk("BC_ERR: 0x%08x\n", mfdcr(DCRN_PLB6_ERR));

		printk("\nPLB6-to-PLB4 Bridge:\n");
		printk("ESR:  0x%08x\n", mfdcr(DCRN_PLB6PLB4_ESR));
		printk("EARH: 0x%08x\n", mfdcr(DCRN_PLB6PLB4_EARH));
		printk("EARL: 0x%08x\n", mfdcr(DCRN_PLB6PLB4_EARL));

		printk("\nPLB4-to-PLB6 Bridge:\n");
		printk("ESR:  0x%08x\n", mfdcr(DCRN_PLB4PLB6_ESR));
		printk("EARH: 0x%08x\n", mfdcr(DCRN_PLB4PLB6_EARH));
		printk("EARL: 0x%08x\n", mfdcr(DCRN_PLB4PLB6_EARL));

		printk("\nPLB6-to-MCIF Bridge:\n");
		printk("BESR0: 0x%08x\n", mfdcr(DCRN_PLB6MCIF_BESR0));
		printk("BESR1: 0x%08x\n", mfdcr(DCRN_PLB6MCIF_BESR1));
		printk("BEARH: 0x%08x\n", mfdcr(DCRN_PLB6MCIF_BEARH));
		printk("BEARL: 0x%08x\n", mfdcr(DCRN_PLB6MCIF_BEARL));

		printk("\nPLB4 Arbiter:\n");
		printk("P0ESRH 0x%08x\n", mfdcr(DCRN_PLB4_P0ESRH));
		printk("P0ESRL 0x%08x\n", mfdcr(DCRN_PLB4_P0ESRL));
		printk("P0EARH 0x%08x\n", mfdcr(DCRN_PLB4_P0EARH));
		printk("P0EARH 0x%08x\n", mfdcr(DCRN_PLB4_P0EARH));
		printk("P1ESRH 0x%08x\n", mfdcr(DCRN_PLB4_P1ESRH));
		printk("P1ESRL 0x%08x\n", mfdcr(DCRN_PLB4_P1ESRL));
		printk("P1EARH 0x%08x\n", mfdcr(DCRN_PLB4_P1EARH));
		printk("P1EARH 0x%08x\n", mfdcr(DCRN_PLB4_P1EARH));

		show_plbopb_regs(DCRN_PLB4OPB0_BASE, 0);
		show_plbopb_regs(DCRN_PLB4OPB1_BASE, 1);
		show_plbopb_regs(DCRN_PLB4OPB2_BASE, 2);
		show_plbopb_regs(DCRN_PLB4OPB3_BASE, 3);

		printk("\nPLB4-to-AHB Bridge:\n");
		printk("ESR:   0x%08x\n", mfdcr(DCRN_PLB4AHB_ESR));
		printk("SEUAR: 0x%08x\n", mfdcr(DCRN_PLB4AHB_SEUAR));
		printk("SELAR: 0x%08x\n", mfdcr(DCRN_PLB4AHB_SELAR));

		printk("\nAHB-to-PLB4 Bridge:\n");
		printk("\nESR: 0x%08x\n", mfdcr(DCRN_AHBPLB4_ESR));
		printk("\nEAR: 0x%08x\n", mfdcr(DCRN_AHBPLB4_EAR));
		panic("Bus Error\n");
	} else if (of_device_is_compatible(np, FSP2_CMU_ERR)) {
		printk("CMU Error\n");
		printk("FIR0: 0x%08x\n", mfcmu(CMUN_FIR0));
		panic("CMU Error\n");
	} else if (of_device_is_compatible(np, FSP2_CONF_ERR)) {
		printk("Configuration Logic Error\n");
		printk("CONF_FIR: 0x%08x\n", mfdcr(DCRN_CONF_FIR_RWC));
		printk("RPERR0:   0x%08x\n", mfdcr(DCRN_CONF_RPERR0));
		printk("RPERR1:   0x%08x\n", mfdcr(DCRN_CONF_RPERR1));
		panic("Configuration Logic Error\n");
	} else if (of_device_is_compatible(np, FSP2_OPBD_ERR)) {
		panic("OPBD Error\n");
	} else if (of_device_is_compatible(np, FSP2_MCUE)) {
		printk("DDR: Uncorrectable Error\n");
		printk("MCSTAT:            0x%08x\n",
			mfdcr(DCRN_DDR34_BASE + DCRN_DDR34_MCSTAT));
		printk("MCOPT1:            0x%08x\n",
			mfdcr(DCRN_DDR34_BASE + DCRN_DDR34_MCOPT1));
		printk("MCOPT2:            0x%08x\n",
			mfdcr(DCRN_DDR34_BASE + DCRN_DDR34_MCOPT2));
		printk("PHYSTAT:           0x%08x\n",
			mfdcr(DCRN_DDR34_BASE + DCRN_DDR34_PHYSTAT));
		printk("CFGR0:             0x%08x\n",
			mfdcr(DCRN_DDR34_BASE + DCRN_DDR34_CFGR0));
		printk("CFGR1:             0x%08x\n",
			mfdcr(DCRN_DDR34_BASE + DCRN_DDR34_CFGR1));
		printk("CFGR2:             0x%08x\n",
			mfdcr(DCRN_DDR34_BASE + DCRN_DDR34_CFGR2));
		printk("CFGR3:             0x%08x\n",
			mfdcr(DCRN_DDR34_BASE + DCRN_DDR34_CFGR3));
		printk("SCRUB_CNTL:        0x%08x\n",
			mfdcr(DCRN_DDR34_BASE + DCRN_DDR34_SCRUB_CNTL));
		printk("ECCERR_PORT0:      0x%08x\n",
			mfdcr(DCRN_DDR34_BASE + DCRN_DDR34_ECCERR_PORT0));
		printk("ECCERR_ADDR_PORT0: 0x%08x\n",
			mfdcr(DCRN_DDR34_BASE + DCRN_DDR34_ECCERR_ADDR_PORT0));
		printk("ECCERR_CNT_PORT0:  0x%08x\n",
			mfdcr(DCRN_DDR34_BASE + DCRN_DDR34_ECCERR_COUNT_PORT0));
		printk("ECC_CHECK_PORT0:   0x%08x\n",
			mfdcr(DCRN_DDR34_BASE + DCRN_DDR34_ECC_CHECK_PORT0));

		printk("MCER0:            0x%08x\n",
			mfdcr(DCRN_CW_BASE + DCRN_CW_MCER0));
		printk("MCER1:            0x%08x\n",
			mfdcr(DCRN_CW_BASE + DCRN_CW_MCER1));

		printk("BESR:             0x%08x\n", mfdcr(DCRN_PLB6MCIF_BESR0));
		printk("BEARL:            0x%08x\n", mfdcr(DCRN_PLB6MCIF_BEARL));
		printk("BEARH:            0x%08x\n", mfdcr(DCRN_PLB6MCIF_BEARH));
		panic("DDR: Uncorrectable Error\n");
	} else if (of_device_is_compatible(np, FSP2_RST_WRN)) {
		crcs = mfcmu(CMUN_CRCS);
		switch (crcs & CRCS_STAT_MASK) {
			case CRCS_STAT_CHIP_RST_B:
				panic("Received chassis-initiated reset request");
			default:
				panic("Unknown external reset: CRCS=0x%x", crcs);
		}
	}
	return IRQ_HANDLED;
}

static void node_irq_request(const char *compat)
{
	struct device_node *np;
	irq_handler_t handler = error_irq_handler;
	unsigned int irq;
	int32_t rc;

	for_each_compatible_node(np, NULL, compat) {
		irq = irq_of_parse_and_map(np, 0);
		if (irq == NO_IRQ) {
			printk("device tree node %s is missing"
				" a valid interrupt", np->name);
			return;
		}

		rc = request_irq(irq, handler, 0, np->name, np);
		if (rc) {
			printk("fsp_of_probe: request_irq failed: np=%s rc=%d",
			      np->full_name, rc);
			return;
		}
	}
}

static void critical_irq_setup(void)
{
	node_irq_request(FSP2_CMU_ERR);
	node_irq_request(FSP2_BUS_ERR);
	node_irq_request(FSP2_CONF_ERR);
	node_irq_request(FSP2_OPBD_ERR);
	node_irq_request(FSP2_MCUE);
	node_irq_request(FSP2_RST_WRN);
}

static int __init fsp2_device_probe(void)
{
	of_platform_bus_probe(NULL, fsp2_of_bus, NULL);
	return 0;
}
machine_device_initcall(fsp2, fsp2_device_probe);

static int __init fsp2_probe(void)
{
	unsigned long root = of_get_flat_dt_root();
	u32 val = 0;

	if (!of_flat_dt_is_compatible(root, "ibm,fsp2"))
		return 0;

	/* Clear BC_ERR and mask snoopable request plb errors. */
	val = mfdcr(DCRN_PLB6_CR0);
	val |= 0x20000000;
	mtdcr(DCRN_PLB6_BASE, val);
	mtdcr(DCRN_PLB6_HD, 0xffff0000);
	mtdcr(DCRN_PLB6_SHD, 0xffff0000);

	/* TVSENSE reset is blocked (clock gated) by the POR default of the TVS
	 * sleep config bit. As a consequence, TVSENSE will provide erratic
	 * sensor values, which may result in spurious (parity) errors
	 * recorded in the CMU FIR and leading to erroneous interrupt requests
	 * once the CMU interrupt is unmasked.
	 */

	/* 1. set TVS1[UNDOZE] */
	val = mfcmu(CMUN_TVS1);
	val |= 0x4;
	mtcmu(CMUN_TVS1, val);

	/* 2. clear FIR[TVS] and FIR[TVSPAR] */
	val = mfcmu(CMUN_FIR0);
	val |= 0x30000000;
	mtcmu(CMUN_FIR0, val);

	/* L2 machine checks */
	mtl2(L2PLBMCKEN0, 0xffffffff);
	mtl2(L2PLBMCKEN1, 0x0000ffff);
	mtl2(L2ARRMCKEN0, 0xffffffff);
	mtl2(L2ARRMCKEN1, 0xffffffff);
	mtl2(L2ARRMCKEN2, 0xfffff000);
	mtl2(L2CPUMCKEN,  0xffffffff);
	mtl2(L2RACMCKEN0, 0xffffffff);
	mtl2(L2WACMCKEN0, 0xffffffff);
	mtl2(L2WACMCKEN1, 0xffffffff);
	mtl2(L2WACMCKEN2, 0xffffffff);
	mtl2(L2WDFMCKEN,  0xffffffff);

	/* L2 interrupts */
	mtl2(L2PLBINTEN1, 0xffff0000);

	/*
	 * At a global level, enable all L2 machine checks and interrupts
	 * reported by the L2 subsystems, except for the external machine check
	 * input (UIC0.1).
	 */
	mtl2(L2MCKEN, 0x000007ff);
	mtl2(L2INTEN, 0x000004ff);

	/* Enable FSP-2 configuration logic parity errors */
	mtdcr(DCRN_CONF_EIR_RS, 0x80000000);

	return 1;
}

static void __init fsp2_irq_init(void)
{
	uic_init_tree();
	critical_irq_setup();
}

define_machine(fsp2) {
	.name			= "FSP-2",
	.probe			= fsp2_probe,
	.progress		= udbg_progress,
	.init_IRQ		= fsp2_irq_init,
	.get_irq		= uic_get_irq,
	.restart		= ppc4xx_reset_system,
	.calibrate_decr		= generic_calibrate_decr,
};
