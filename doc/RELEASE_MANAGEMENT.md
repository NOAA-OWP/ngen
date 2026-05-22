# Release Management

The page discusses the release process for official versions of _ngen_.  This process is very much interrelated to the repo branching management model, as discussed in detail on the [GIT_USAGE](./GIT_USAGE.md) doc.

# The Release Process

## TL;DR

The release process can be summarized fairly simply:
- A version name is finalized
- A release candidate branch is created
- Testing, QA, fixes are done on the release candidate branch
- Once release candidate is ready, changes are integrated into `production` and `master`, and the new version is tagged

## Process Steps


[comment]: <> (TODO: Document release manual testing and QA procedures)
[//]: # (TODO: document testing and quality checks/process for release candidate prior to release)
[//]: # (TODO: document peer review and integration process for bug fixes, doc updates, etc., into release candidate branch prior to release (i.e, regular PR?)

1. The next version number is decided/finalized
    - Version numbering should follow [Semantic Versioning](https://semver.org/) and its typical `MAJOR.MINOR.PATCH` pattern
2. A release candidate branch, based on `master`, is created in the official OWP repo
    - The name of this branch will be `release-X` for version `X`
3. The version is incremented in the main [CMakeLists.txt](../CMakeLists.txt)
    - Update the line setting the version, which will look something like `project(ngen VERSION x.y.z)`
    - Then committed and pushed this change to the `release-X` branch
4. All necessary testing and quality pre-release tasks are performed using this release candidate branch
    - **TODO**: to be documented in more detail
4. (If necessary) Bug fixes, documentation updates, and other acceptable, non-feature changes are applied to the release branch
    - Such changes should go through some peer review process before inclusion in the official OWP branch (e.g., PRs, out-of-band code reviews, etc.)
    - **TODO**: process to be decided upon and documented
5. Steps 3. and 4. are repeated as needed until testing, quality checks, etc. in Step 3. do not require another iteration of Step 4.
    - At this point, the branch is ready for official release
6. All changes in the release candidate branch are incorporated into `production` in the official OWP repo
    - Note that **rebasing** should be used to reconcile changes ([see here](../CONTRIBUTING.md#a-rebase-strategy) for more info)
7. The subsequent `HEAD` commit of `production` is tagged with the new version in the official OWP repo
8. All changes in the release candidate branch are incorporated back into `master` in the official OWP repo
    - This will include things like bug fixes committed to `release-X` after it was branched from `master`
    - As with `production` in Step 6., this should be [done using rebasing](../CONTRIBUTING.md#a-rebase-strategy)
9. The release candidate branch is deleted from the OWP repo (and, ideally, other clones and forks)