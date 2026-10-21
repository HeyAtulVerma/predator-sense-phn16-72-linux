# Security policy

`predator-sensed` runs as root and the `predator_wmi` module exposes firmware methods to root, so security issues
matter. Please **do not open a public issue** for vulnerabilities: use GitHub's
[private vulnerability reporting](https://github.com/aishend/predator-sense-phn16-72-linux/security/advisories/new).

In scope: privilege escalation through the D-Bus API or polkit policy, ways to reach WMI methods outside the
allow-list, memory-safety bugs in the kernel module or service, unsafe file handling in the state directory.
