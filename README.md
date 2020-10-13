# PTPF for developers

Hi, you should not see this text on the master branch. This is the README for developers. It should be (and stay) about development related information.

# How to fumble a new release

A "released version" should be on master with the commit message formatted like so..  
"* updated master to \<major\>.\<minor>.\<patch\>"

To release the binaries for that version, make a commit over @ [ptpf-downloads/master](https://github.com/Wallby/ptpf-downloads), with the commit message formatted like so..  
"\+ added \<linux|windows\> (\<32-bit|64-bit\>) binaries and headers for \<ptpf-loader|ptpf-server\> \<major\>.\<minor\>.\<patch\>".

make a Github-release on [the-Github-page](https://github.com/Wallby/ptpf-downloads) with a..  
.. .zip that includes the Windows binaries and the platform agnostic headers (if there is a windows build for the version)  
.. .tar that includes the Linux binaries and the platform agnostic headers (if there is a linux build for the version)  
<!-- maybe when support for Android becomes "feasible" add a binary for that too -->

NOTE: at all times must the README.md on [ptpf-downloads/master](https://github.com/Wallby/ptpf-downloads) look like so..
```
# PTPF stands for Paralel Thread Processing Framework

The goal is to make this a library that can be included by just linking against libptpf. For specific functionality, include specific .h files.

Version: \<major\>.\<minor\>.\<patch\>

# Feel free to contribute!

Contributors..
.. \<contributor\>
```

The text above may be updated with progression of ptpf development.
