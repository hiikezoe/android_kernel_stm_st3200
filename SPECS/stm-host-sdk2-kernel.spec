Name:		%{_stm_pkg_prefix}-host-sdk2-kernel-source-3.x-%{_stm_target_name}
Obsoletes: %{_stm_pkg_prefix}-havana-kernel < 2.6.32.28_stm24_208
Obsoletes: %{_stm_pkg_prefix}-host-havana-kernel-source-%{_stm_target_name}
%define _kernel_ver 0302
Version:	3.4.7_stm%{_stm_short_build_id}_%{_kernel_ver}
Release:	11
License:	GPL

URL:		http://www.stlinux.com
Source0:	stm-sdk2-kernel.tar.bz2

Buildroot:	%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
Prefix:		%{_stm_install_prefix}


BuildArch: noarch

Summary: SDK2 kernel package
Group: Development/Source
AutoReqProv: no
Requires: %{_stm_pkg_prefix}-host-filesystem
%description
 Top level kernel package for SDK2.


%prep
%setup -qcn sdk2-linux-%{_stm_target_name}-%{version}-%{release}


%build
# nothing to do


%install
rm -rf .git*

chmod -R u=rwX,go=rX .
install -d -m 755 %{buildroot}%{_stm_kernel_dir}/sdk2-linux-%{_stm_target_name}-%{version}-%{release}
tar chf - . | tar xf - -C %{buildroot}%{_stm_kernel_dir}/sdk2-linux-%{_stm_target_name}-%{version}-%{release}

# set localversion-stm
rm -f %{buildroot}%{_stm_kernel_dir}/sdk2-linux-%{_stm_target_name}-%{version}-%{release}/localversion*
echo '_'%(expr %{version}-%{release}  : '[^_]*_\(.*\)') > \
  %{buildroot}%{_stm_kernel_dir}/sdk2-linux-%{_stm_target_name}-%{version}-%{release}/localversion-stm


%clean
rm -rf %{buildroot}


%files
%defattr(-,root,root)
%docdir %{_stm_kernel_dir}/sdk2-linux-%{_stm_target_name}-%{version}-%{release}/Documentation
%{_stm_kernel_dir}/sdk2-linux-%{_stm_target_name}-%{version}-%{release}


%post
rm -f %{_stm_kernel_dir}/sdk2-linux-3.x-%{_stm_target_name}
ln -s sdk2-linux-%{_stm_target_name}-%{version}-%{release} %{_stm_kernel_dir}/sdk2-linux-3.x-%{_stm_target_name}


%preun
if [ x`readlink %{_stm_kernel_dir}/sdk2-linux-3.x-%{_stm_target_name}` = xsdk2-linux-%{_stm_target_name}-%{version}-%{release} ] ; then
  rm -f %{_stm_kernel_dir}/sdk2-linux-3.x-%{_stm_target_name}
fi


%changelog
* Thu Nov 29 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.4.7_stm24_0302-11
- [Bugzilla: 24721] add PM support to i2c-gpio

* Mon Nov 26 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.4.7_stm24_0302-10
- [Bugzilla: 23852] minor follow-up change
- [Bugzilla: 24169] add PM support to STM NAND FLEX driver
- [Bugzilla: 22613] HoM doesn't work for more than 30 seconds
- [Bugzilla: 24173] WoL work-around for B2020
- [Bugzilla: 22658] stm:fdma: fix HoM support in the driver
- fix some compiler warnings
- rtc alarm fix

* Thu Nov 08 2012 Daniel Thompson <daniel.thompson@st.com> - sdk2-linux-${_stm_target_name}-3.4.7_stm24_0302-9
- [Update] incorporate latest STLinux updates

* Wed Nov 07 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.4.7_stm24_0302-8
- b2078: configure the FastPath interface
- [Bugzilla: 23852] make SBC config dynamic based on board
- update STiG125 PM support
- update STM AHCI PM support

* Thu Nov 01 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.4.7_stm24_0302-7
- [Update] incorporate latest STLinux updates
- amongst others, this gives us the fastpath driver and b2078 board support
- b2000/b2020: update TSin gpio configuration

* Wed Oct 24 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.4.7_stm24_0302-6
- [Update: 3.4.7_stm24_0302-5] merge in latest CPT updates

* Wed Oct 17 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.4.7_stm24_0302-5
- [Bugzilla: 22658] FDMA restore does not work
- [Bugzilla: 23427] restore TSIN sysconf registers after HoM
- [Bugzilla: 23404] stm: sata: Fixed PM_Runtime issue
- [Spec] fix some changelogs

* Mon Oct 15 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.4.7_stm24_0302-4
- [Bugzilla: 23207] workaround for big kernel images not booting

* Fri Oct 05 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.4.7_stm24_0302-3
- [Bugzilla: 22393] snd: fix application of oversampling frequency adjustment

* Tue Oct 02 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.4.7_stm24_0302-2
- [Bugzilla: 22658] FDMA driver updates for HoM
- [Bugzilla: 22811] fix refcounting for ALWAYS_ENABLED clocks for CPS

* Tue Sep 25 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.4.7_stm24_0302-1
- [Update: 3.4.7_stm24_0302; Bugzilla: 22254] update to latest version
- [Bugzilla: 17870] improve fb_find_mode() best match logic when interlaced modes exist

* Thu Sep 20 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.3.1_stm24_0301-15
- [Bugzilla: 22616] stm:pm:hom: don't set DDR power pins via sysconf on CPS
- [Bugzilla: 20733] stm:pm:hom: fix WoL for CPS

* Wed Sep 19 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.3.1_stm24_0301-14
- [Bugzilla: 21948] bpa2 partition creation fails with non contiguous system ram
- [Bugzilla: 20433] further alsa / fdma fixes

* Wed Sep 12 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.3.1_stm24_0301-13
- [Bugzilla: 18910] Linux HoM: HoM tag address should be physical
- [Bugzilla: 22074] add LPM driver
- [Bugzilla: 22075] RTC support for CPS (RTC-SBC)
- [Bugzilla: 22077] wakeup devices notification
- please note that for correct CPS operation the LPM firmware is needed

* Fri Sep 07 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.3.1_stm24_0301-12
- [Bugzilla: 21478] stm/ir: fix NULL pointer dereference in irq handler
- stm/ir: fix oops during boot caused by the patched from bug 22079

* Thu Sep 06 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.3.1_stm24_0301-11
- [Update] incorporate lots of STLinux updates
- [Bugzilla: 18688] enable I2S input on B2000 via config option
- [Bugzilla: 20335] first batch of related patches
- [Bugzilla: 21563] Null ptr deref in snd_pcm_period_elapsed()
- [Bugzilla: 21472] Crash in stm_fdma_tasklet_complete()
- [Bugzilla: 22074] first batch of related patches (via STLinux)
- [Bugzilla: 22079] LIRC fixes (via STLinux)

* Thu Aug 16 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.3.1_stm24_0301-10
- [Update] Update the STLinux wavefront - clock LLA, FDMA, ALSA, L2 cache config, spi updates,
  fix compiler warnings in ST drivers
- update mali driver to r2p4
- [Bugzilla: 20905] fdma: sleeping function called from invalid context
- [Bugzilla: 18070] stm:fli7610: add PCIe Support

* Wed Aug 08 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.3.1_stm24_0301-9
- [Bugzilla: 21387] stm:b2000: correct typo with tsin configuration
- [Bugzilla: 19376] stm:snd:fli7610 Configuration for loud speaker support
- [Bugzilla: 20638] stm: fli7610 and fli76xxhdk: Fix SSC problems

* Fri Aug 03 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.3.1_stm24_0301-8
- [Update] Update the STLinux wavefront - lots of stmmac fixes, sdhci-pm fixes,
  fix compiler warnings in ST drivers
- [Bugzilla: 21212] Kernel Oops in running smoked tests
- [Bugzilla: 20405] stm: fli7610: add missing GPIO irqmux devices
- stm: stih415: clk: code tidy-up to remove WARN_ON()s highlighted by changes through Bugzilla 21112

* Wed Aug 01 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.3.1_stm24_0301-7
- [Bugzilla: 21112] stm: clk: prevent unbalanced calls to clk_disable()
- [Bugzilla: 20903] stm:fdma Fix for FDMA driver error interrupt path BUG_ON
- [Bugzilla: 20903] stm:snd Do not activate parking mode on XRUN
- [Bugzilla: 21039] wrong uImage generated for b2020
- [Bugzilla: 20967] disable L2 cache during HPS
- [Bugzilla: 20937] new sections mismatch warnings since 0301-5

* Fri Jul 20 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.3.1_stm24_0301-6
- [Bugzilla: 18871] b2000: configure pads for tsin2 - tsin5
- [Bugzilla: 20707] b2020: enable GPIO bitbang for SSC0 and configure additional pads for NIM-A and NIM-B
- [Bugzilla: 19641] stm_sata: Handle spurious irqs better

* Thu Jul 12 2012 Daniel Thompson <daniel.thompson@st.com> - sdk2-linux-%{_stm_target_name}-3.3.1_stm24_0301-5
- [Update; Bugzilla: 17728; Bugzilla: 18850] Update the STLinux wavefront, mostly to get updated DMA API.
- [Bugzilla: 19642] STiH415/audio: Update channel status bits correctly
- [Bugzilla: 20197] ASC driver bug fixes
- [Bugzilla: 20293] Update clock LLA and fix clock framework parenting bugs
- [Bugzilla: 19403; Bugzilla 19404] Improve co-processor firmware loader

* Fri Jun 08 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.3.1_stm24_0301-4
- [Bugzilla: 19738] no write permission for runtime resume/suspend sysfs entries

* Thu Jun 07 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.3.1_stm24_0301-3
- [Bugzilla: 19437] add board support for B2020 / STiH415
- [Bugzilla: 19555] dvb-core: fix DVBFE_ALGO_HW retune bug (backported from linux 3.3.2)

* Wed May 30 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.3.1_stm24_0301-2
- [Bugzilla: 15140] stmmac: fix suspend/resume locking
- stm: make WARNON_RATELIMIT have a burst of 1 per 5 seconds
- added latest patches from CPT

* Wed May 23 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.3.1_stm24_0301-1
- [Bugzilla: 18856] Upgrade kernel to 3.3

* Mon May 21 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.2.2_stm24_0300-9
- [Bugzilla: 19041] disabled PM causes USB to break
- [Bugzilla: 18688] Correct uniperipheral reader data clocking edge
- [Bugzilla: 18777] Allow 8kHz sample rate for uniperipheral drivers
- [Bugzilla: 16689] Fix uniperipheral config for layout1 transmission
- [Bugzilla: 18042] UART1 on Orly validation board outputs junk character on console
- [Bugzilla: 19128] all external modules taint the kernel

* Tue May 15 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.2.2_stm24_0300-8
- [Bugzilla: 19031] stm: fli7610: mali: enable mali driver on Newman

* Wed May 09 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.2.2_stm24_0300-7
- [Bugzilla: 16509] merge PM support into kernel

* Tue May 08 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.2.2_stm24_0300-6
- [Bugzilla: 18534, 18646] various LiRC issues
- [Bugzilla: 18825] kernel crashes while booting during during stm_amba_bridge_init()
- fix Newman VTAC problem

* Mon Apr 30 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.2.2_stm24_0300-5
- [Bugzilla: 18010] stm:fli7610:lirc: Add sbc_comms_clk alias
- [Bugzilla: 18095] stm:snd:fli7610 Power down AATV before configuring

* Thu Apr 12 2012 Kieran Bingham <kieran.bingham@st.com> - sdk2-linux-%{_stm_target_name}-3.2.2_stm24_0300-4
- [Bugzilla: 17387] stm:ir: Add missing logic to handle Overrun
- [Bugzilla: 17387] ir-raw: Check available elements in kfifo before adding.
- [Bugzilla: 18176] ARM: Increase COMMAND_LINE_SIZE to cope with longer command lines
- [Bugzilla: 17100] bpa2: Allow BPA2 partitions to be contiguous with kernel memory

* Mon Apr 02 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.2.2_stm24_0300-3
- [Bugzilla: 18047] disable audio parking buffer

* Thu Mar 15 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.2.2_stm24_0300-2
- [Update; Bugzilla: 17355] merge in latest STLinux patches, mainly to get the ALSA patches

* Tue Mar 06 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.2.2_stm24_0300-1
- [Update: 3.2.2] update to Linux-3.2.2

* Tue Feb 21 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.0.0+3.1.0rc9_stm24_0300-5
- stm: mm: Replace WARN_ON when overmapping RAM with simple printk to make SMP kernel work
- stm: pad: Export stm_pad_gpio_request_input

* Fri Feb 17 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.0.0+3.1.0rc9_stm24_0300-4
- [Bugzilla: 15140] PM support for any CONFIG_VMSPLIT_xG option
- fix alsa SMP deadlocks
- mm: export set_iounmap_nonlazy() which is used by latest multicom version

* Fri Feb 10 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.0.0+3.1.0rc9_stm24_0300-3
- [Bugzilla: 16823] Live decode from satellite fails

* Mon Feb 06 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.0.0+3.1.0rc9_stm24_0300-2
- [Bugzilla: 16283, 16554] STiH415: FVDP clocks are not set properly
- pull in latest changes from Dave's 3.x kernel
- fix LIRC

* Fri Jan 20 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-3.0.0+3.1.0rc9_stm24_0300-1
- [Update: 3.1.0rc9] update to 3.1.0rc9

* Fri Jan 20 2012 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-2.6.32.42_stm24_0208-18
- [Bugzilla: 16175] USB doesn't work after compiler upgrade

* Thu Dec 22 2011 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-2.6.32.42_stm24_0208-17
- [Update: git 85971b05b] merge in latest changes from STLinux
- This adds amongst others: kptrace support, L2 cache and SMP fixes, clk fixes,
  ARM errata workarounds

* Wed Dec 14 2011 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-2.6.32.42_stm24_0208-16
- [Bugzilla: 15057, 15863] fix OHCI support on STiH415

* Wed Dec 07 2011 André Draszik <andre.draszik@st.com> - sdk2-linux-%{_stm_target_name}-2.6.32.42_stm24_0208-15
- [Bugzilla: 15652; Spec] Remove product codenames from kernel package.

* Thu Oct 20 2011 Daniel Thompson <daniel.thompson@st.com> - havana-linux-%{_stm_target_name}-2.6.32.42_stm24_0208-14
- [Bugzilla: 13979] uniperipheral: resolve underflow every 50s when operated
  in double buffered mode.

* Tue Oct 11 2011 Daniel Thompson <daniel.thompson@st.com> - havana-linux-%{_stm_target_name}-2.6.32.42_stm24_0208-13
- [Bugzilla: 14623] Update Mali driver to latest r2p2-03rev0 release
- [Bugzilla: 14381] uniperipheral: explicitly disable SPDIF formatter

* Mon Sep 19 2011 Daniel Thompson <daniel.thompson@st.com> - havana-linux-%{_stm_target_name}-2.6.32.42_stm24_0208-12
- [Bugzilla: 14242] board-b2000: Re-instate configure_tsin()

* Thu Sep 15 2011 Daniel Thompson <daniel.thompson@st.com> - havana-linux-%{_stm_target_name}-2.6.32.42_stm24_0208-11
- [Bugzilla: 13954] bpa2: Fix low memory reservation on ARM systems.

* Tue Sep 13 2011 Daniel Thompson <daniel.thompson@st.com> - havana-linux-%{_stm_target_name}-2.6.32.42_stm24_0208-10
- [Bugzilla: 14121] Update the pre-release ALSA patches for STiH415.

* Thu Sep  8 2011 André Draszik <andre.draszik@st.com> - havana-linux-%{_stm_target_name}-2.6.32.42_stm24_0208-9
- [Bugzilla: 13996] Add tsin pins pad configuration to ARM kernel

* Wed Aug 31 2011 André Draszik <andre.draszik@st.com> - havana-linux-%{_stm_target_name}-2.6.32.42_stm24_0208-8
- add usleep_range() timer
- [Bugzilla: 13635] add stv0367 driver

* Wed Aug 24 2011 Daniel Thompson <daniel.thompson@st.com> - havana-linux-%{_stm_target_name}-2.6.32.42_stm24_0208-7
- [Bugzilla: 13778] Integrate pre-release ALSA patches for STiH415.

* Wed Aug 17 2011 André Draszik <andre.draszik@st.com> - havana-linux-%{_stm_target_name}-2.6.32.42_stm24_0208-6
- [Bugzilla: 13739] Add support for Thomson DTT7546X tuner found on STV0367 NIM

* Wed Aug 10 2011 André Draszik <andre.draszik@st.com> - havana-linux-%{_stm_target_name}-2.6.32.42_stm24_0208-5
- [Spec] cosmetic updates taken from SH4 version, rename SRPM

* Tue Aug 09 2011 André Draszik <andre.draszik@st.com> - havana-linux-%{_stm_target_name}-2.6.32.42_stm24_0208-4
- [Update: 2.6.32.28-208+any-20110809; Bugzilla 13621] merge in latest STLinux pre-release kernel
- [Bugzilla: 13621] add updated STM coprocessor patch

* Thu Aug 4 2011 Daniel Thompson <daniel.thompson@st.com> - havana-linux-%{_stm_target_name}-2.6.32.42_stm24_0208-3
- [Bugzilla: 13272] Export additional kernel symbols used by player2.

* Tue Aug 2 2011 Daniel Thompson <daniel.thompson@st.com> - havana-linux-%{_stm_target_name}-2.6.32.42_stm24_0208-2
- [Update: 2.6.32.28-208+any-20110802] merge in latest STLinux pre-release kernel

* Tue Jul 19 2011 André Draszik <andre.draszik@st.com> - havana-linux-%{_stm_target_name}-2.6.32.42_stm24_0208-1
- [Spec] update to STLinux 208 (without any shark patches!)
- [Spec] major cleanup: sh4 vs. %%{_stm_target_name}

* Wed Jun 29 2011 André Draszik <andre.draszik@st.com> - havana-linux-sh4-2.6.32.28_stm24_0207-5
- [Bugzilla: 12726] hdk7108: new BPA2 partition layout to support dual HD decode

* Tue Jun 21 2011 André Draszik <andre.draszik@st.com> - havana-linux-sh4-2.6.32.28_stm24_0207-4
- [Bugzilla: 12719] update kernel with latest mali driver
- V4L/DVB: stv090x: change default routing

* Thu Jun 02 2011 André Draszik <andre.draszik@st.com> - havana-linux-sh4-2.6.32.28_stm24_0207-3
- [Bugzilla: 12446] applications randomly crash on startup
- [Bugzilla: 11229] sh: fixed issues in atomic exchange functions

* Tue May 17 2011 André Draszik <andre.draszik@st.com> - havana-linux-sh4-2.6.32.28_stm24_0207-2
- [Bugzilla: 11355] add support for Freeman (stslave and BPA2)
- hdk7108: update mali memory configuration for havana

* Mon May 16 2011 André Draszik <andre.draszik@st.com> - havana-linux-sh4-2.6.32.28_stm24_0207-1
- [Update: 2.6.32.28-206] merge in latest STLinux version 2.6.32.28-207
- hdk7108: Add pad manager configuration for tsin0 and tsin1 pins
- sh_stm: cb180: Use the new MAC/PHY configuration parameters
- stm_mali: cleanup Makefile and fix out of tree builds
- [Bugzilla: 11113] lkm_sh: handle R_SH_NONE relocations in modules
- rtc_stm_lpc: properly handle clk_get failure on missing clock

* Tue May 03 2011 André Draszik <andre.draszik@st.com> - havana-linux-sh4-2.6.32.28_stm24_0206-9
- [Bugzilla: 10952, 11393, 11526] port XFS fixes from 2.6.34 kernel

* Tue May 03 2011 André Draszik <andre.draszik@st.com> - havana-linux-sh4-2.6.32.28_stm24_0206-8
- [Bugzilla: 12059] install more of the STM kernel headers

* Fri Apr 15 2011 Peter Griffin <peter.griffin@st.com> - havana-linux-sh4-2.6.32.28_stm24_0206-7
- [Bugzilla: 11820] Add DVB_PLL_THOMSON_DTT7546X tuner support

* Tue Apr  5 2011 André Draszik <andre.draszik@st.com> - havana-linux-sh4-2.6.32.28_stm24_0206-6
- [Bugzilla: 11229] sh: fixed issue in xchg_u32 function
- gconfig: fix build failure on fedora 13
- stm: stm_device: Fix kernel fault freeing devres allocated memory.

* Tue Mar 22 2011 André Draszik <andre.draszik@st.com> - havana-linux-sh4-2.6.32.28_stm24_0206-5
- [Bugzilla: 11212] Fix unaligned access in ilc3 when "ILC: spurious interrupt demux"
- [Bugzilla: 11085] export 'flush_kernel_dcache_page_addr()' function
- [Bugzilla: 11085] initialize the spi->mode bits understood by the spi_stm driver
- [Bugzilla: 11254] Fix USB power control and electrical parameters
- [Bugzilla: 11463] fli75xx: Add missing change to switch fli7510 USB to stm_device API

* Thu Mar 10 2011 Daniel Thompson <daniel.thompson@st.com> - havana-linux-sh4-2.6.32.28_stm24_0206-4
- Add bpa2 support for STi7200/MB671 platforms
- Add clock gen C support for STi7200 devices

* Mon Feb 28 2011 André Draszik <andre.draszik@st.com> - havana-linux-sh4-2.6.32.28_stm24_0206-3
- stm_bios: Add BIOS support and set the Zero page size to 64K if used.

* Thu Feb 24 2011 André Draszik <andre.draszik@st.com> - havana-linux-sh4-2.6.32.28_stm24_0206-3
- add a userspace mmap interface to bpa2
- add interface to search in bpa2

* Tue Feb 22 2011 Kieran Bingham <kieran.bingham@st.com> - havana-linux-sh4-2.6.32_28_stm24_0206-2
- Cherry picked patches for immediate fixes of the following bugs:
- [Bugzilla: 11167] Support stslave loading on 7108
- [Bugzilla: 11254] No more USB on HDK7108 (v1 or v2) using linux-sh4-2.6.32.28_stm24_0206

* Wed Feb 16 2011 André Draszik <andre.draszik@st.com> - havana-linux-sh4-2.6.32.28_stm24_0206-1
- [Update: 2.6.32.28-206] merge in latest STLinux version 2.6.32.28-206

* Sat Feb 12 2011 André Draszik <andre.draszik@st.com> - havana-linux-sh4-2.6.32.16_stm24_0205-4
- [Bugzilla: 10805] add proper ALSA driver to all platforms
- please note that only STx7105 and STx7108 have been tested properly and
  STx7100, Freeman and STx7200 are known _not_ to work because the clock frame
  work is not up to date for these SoCs. STx7111 and STx7141 should work, though.
- in the process, fix a few bugs in the clock frame work drivers/implementation

* Tue Jan 25 2011 André Draszik <andre.draszik@st.com> - havana-linux-sh4-2.6.32.16_stm24_0205-3
- [Bugzilla: 11035] fixes for using make 3.82

* Thu Dec 16 2010 André Draszik <andre.draszik@st.com> - havana-linux-sh4-2.6.32.16_stm24_0205-2
- hdk7108: add bpa2 memory configuration
- [Bugzilla: 10712] sh_stm: fix for pmb_calc() loops forever
- sh_stm: hdk7106 hdk7108: add hack to make linux use large PMBs

* Mon Dec 06 2010 André Draszik <andre.draszik@st.com> - havana-linux-sh4-2.6.32.16_stm24_0205-2
- [Bugzilla: 10677] sh_stm: hdk7105: revert use of the GPIO based I2C driver for HDMI
- [Bugzilla: 10678] sh_stm: fix havana hdk7105 memory partitioning to be identical to STLinux 2.3 again

* Tue Nov 30 2010 André Draszik <andre.draszik@st.com> - havana-linux-sh4-2.6.32.16_stm24_0205-1
- merge in STLinux24 205 kernel
- hdk7106: really use correct bpa2 partitions (taken from STLinux 2.3)
- update hdk7106 defconfig

* Mon Nov 01 2010 André Draszik <andre.draszik@st.com> - havana-linux-sh4-2.6.32.16_stm24_0204-7
- [STx7106] add ALSA support for STx7106

* Thu Oct 28 2010 Andrew Gardner <andrew.gardner@st.com> - havana-linux-sh4-2.6.32.16_stm24_0204-6
- fdma: Add support for extended SPI Channel to drive Dibcom tuner
- sh_stm: Allow Serial Flash Controller FSM to use FDMA transfers
- dibbridge: Add support for FDMA transfers

* Tue Oct 26 2010 Andrew Gardner <andrew.gardner@st.com> - havana-linux-sh4-2.6.32.16_stm24_0204-5
- [hdk 7106] add bpa2 partitions for HDK7106
- [sh-stm] Add support for cb180
- [sh-stm] Add infrastructure for STM serial flash controller in FS mode
- [sh-stm] Add DibBridge driver for cb180
- [sh-stm] Add defconfig for cb180
- [sh-stm] Fixed UART-2 RXD and CTS routing via PIO12 on STi7106

* Thu Oct 14 2010 Andrew Gardner <andrew.gardner@st.com> - havana-linux-sh4-2.6.32.16_stm24_0204-2
- [hdk7105] Correct i2c hdmi address, fix gpio and ssc crash when both are enabled.

* Mon Sep 27 2010 Andrew Gardner <andrew.gardner@st.com> - havana-linux-sh4-2.6.32.16_stm24_0204-1
- [Update: 2.6.32.16_stm24_0204] update to latest STLinux kernel
- [Spec] fix dates

* Mon Aug 9 2010 Peter Bennett <peter.bennett@st.com> - havana-linux-sh4-2.6.32.16_stm24_0202-1
- [hdk7105] HACK: Horrible fix to get PMBs to allocate large chunks of main memory.
- [hdk7105] Add BPA2 memory config, setup HDMI correctly.
- [havana] Add necessary function for Havana to get the monotonic time.
- [alsa] Fix ALSA issues that are required to do playback (this will be replaced when the kernel team properly do ALSA).
- [alsa] Export symbols required for Havana and add snd_find_minor.
- [linuxdvb] Add the extra options need for player2.
- [st-coproc] Added debug support so we can see output from the firmwares.
