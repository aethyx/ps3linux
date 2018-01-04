# Cell SDK 3.1 for Debian (Jessie)

---

## Introduction

As we were interested in using the Cell processor from PS3s for internal research purposes at AETHYX MEDIAE we were in need for the last version of the Cell SDK 3.1 for the Debian platform.

Unfortunately all the packages are only available for Yellow Dog Linux/RHEL 5.x/Fedora 7/9 distributions.

Thus, we had to convert the provided RPMs into Debian .deb packages with the tool "alien". All ppc-RPMs were converted to `*.powerpc.deb`. All ppc64-RPMs to `*.all.deb`.

Herein we provide all packages for installation on Debian systems, in our case Debian 8.x (Jessie).

For questions/enhancements/critique please send an Email with subject "Cell SDK PS3 Debian" to info [at] aethyx [dot] eu.

Disclaimer: we can't provide that this stuff works as intended on PS3 machines but at least we tried. Use at your own risk.

---

## Prerequisites

Most of the files are open-source already and free to use. These are all the files you find in the folders "ydl" and "bsc".

For files residing in "devel" and "extras" these are a mixture of GPL/LGPL licenses. There exists some proprietary stuff from IBM.

Before installing the Debian packages on your PS3 machine, please visit: [https://www.ibm.com/account/profile/us?page=reg](https://www.ibm.com/account/profile/us?page=reg "https://www.ibm.com/account/profile/us?page=reg"). Create an "IBM ID" here first and download the two ISOs for Cell SDK 3.1. In this case you're making sure that you downloaded the ISOs containing the RPMs yourself.

If you want to run Debian GNU/Linux on your PS3 machine visit [https://sourceforge.net/p/redribbon/wiki/Home/](https://sourceforge.net/p/redribbon/wiki/Home/ "https://sourceforge.net/p/redribbon/wiki/Home/") and download the Live ISO.

---

## Installation

Download all four folders containing the *.deb files.

First we make sure to have the same environment on Debian Jessie as we were provided on "Yellow Dog Linux 6.2", the last YDL release in 2009. On your PS3 Debian machine, `cd` to "ydl" and install all those debs with `sudo dpkg -i *.deb`. This may take a while so please be patient.

Secondly we install what was provided by the Barcelona Supercomputer Center. Thus, cd to "bsc" and again `sudo dpkg -i *.deb`. Please be patient while installing.

Thirdly, we install what was provided on the Cell SDK 3.1 ISOs.

Start by changing your directory to "devel". Again, install all the Debian packages with `sudo dpkg -i *.deb`. These are a lot of files, so this may take a while.
Afterwards, do the same inside the directory "extras".

If everything went well and you kept on, you should find all the docs, directories and files on your PS3 Debian machine needed for programming the Cell processor.

Your SDK files will reside in `/opt/cell/sdk`

Back in the days there was also an IDE from Eclipse but it's from 2008. If you want to start it it's available in `/opt/cell/ide/eclipse`. Due to limited RAM on PS3 machines we don't recommend Eclipse though. As you program in C/C++ most of the time choose an editor of your choice and compile your files manually. Or choose an IDE which can be used in LXDE/XFCE/etc.

---

## Contributing

PowerPC is a dying architecture in general. Debian 8 (Jessie) will probably be the last Debian ever for PowerPC machines. Thus, finding help or contributing to development is a difficult and time-consuming task.

Nevertheless, we provide what helped us sometimes in setting stuff up. It will also come in handy if you want to mail or chat directly with PS3 and/or PowerPC developers (which still exist and work until today):

* Linux PPC-Development: https://lists.ozlabs.org/listinfo/linuxppc-dev (mailing list)
* FREENODE IRC: #debianppc, #psugnd
* OFTC IRC: #cell

---

## Documentation and further reading

* Official Cell SDK 3.1 documentation by IBM: [Cell SDK 3.1 documentation](https://github.com/aethyx/ps3linux/tree/master/Cell%20SDK%203.1%20Dokumentationen "Cell SDK 3.1 documentation")
* Practical Computing on the Cell Broadband Engine by Sandeep Koranne: [Practical Computing on the Cell Broadband Engine](https://dl.acm.org/citation.cfm?id=1594901 "Practical Computing on the Cell Broadband Engine")
* Programming the Cell Broadband Engine Architecture: Examples and Best Practices by IBM: [Programming the Cell Broadband Engine Architecture: Examples and Best Practices](http://www.redbooks.ibm.com/abstracts/sg247575.html "Programming the Cell Broadband Engine Architecture: Examples and Best Practices")
* PS3 Developer Wiki: [http://www.psdevwiki.com/ps3/Cell_Programming_IBM](http://www.psdevwiki.com/ps3/Cell_Programming_IBM "http://www.psdevwiki.com/ps3/Cell_Programming_IBM")
* Cell SPE toolchain by jk.ozlabs.org (outdated): [http://jk.ozlabs.org/docs/cell-spe-toolchain/](http://jk.ozlabs.org/docs/cell-spe-toolchain/ "http://jk.ozlabs.org/docs/cell-spe-toolchain/")
 
