# Contributing to ISA-L

Everyone is welcome to contribute. Patches may be submitted using GitHub pull
requests (PRs). All commits must be signed off by the developer (--signoff)
which indicates that you agree to the Developer Certificate of Origin.  Patch
discussion will happen directly on the GitHub PR. Design pre-work and general
discussion occurs on the [mailing list]. Anyone can provide feedback in either
location and all discussion is welcome. Decisions on whether to merge patches
will be handled by the maintainer.

## License

ISA-L is licensed using a BSD 3-clause [license]. All code submitted to
the project is required to carry that license.

## Certificate of Origin

In order to get a clear contribution chain of trust we use the
[signed-off-by language] used by the Linux kernel project.

## Mailing List

Contributors and users are welcome to submit new request on our roadmap, submit
patches, file issues, and ask questions on our [mailing list].

## Coding Style

The coding style for ISA-L C code roughly follows linux kernel guidelines.  Use
the included indent script to format C code.

    ./tools/iindent your_files.c

And use check format script before submitting.

    ./tools/check_format.sh

[mailing list]:https://lists.01.org/hyperkitty/list/isal@lists.01.org/
[license]:LICENSE
[signed-off-by language]:https://01.org/community/signed-process
