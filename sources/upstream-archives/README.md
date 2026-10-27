# Upstream source trees

The original release archives in this directory were replaced with source
trees so that no oversized archive blob is stored in the parent repository.

Thirty projects with a matching GitHub commit or release tag are pinned as
Git submodules. Their repository URLs are recorded in the top-level
`.gitmodules` file, and the gitlink stored by this repository fixes the exact
commit. Populate them after cloning with:

```sh
git submodule update --init --recursive
```

The following nine projects did not have a sufficiently authoritative GitHub
match for the supplied archive and are therefore committed as extracted source
trees:

- conntrack-tools 1.4.6
- dnsmasq 2.85
- ebtables 2018-06-27 (48cff25d)
- ethtool 5.10
- ipset 7.6
- iptables 1.8.7
- iw 5.19
- mmc-utils 2019-10-10 (73d6c59a)
- ntpclient 2015_365

Directory names retain the archive version or source commit suffix used by the
original disclosure package.
