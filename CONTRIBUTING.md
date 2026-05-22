# Guidance on How to Contribute

> All contributions to this project will be released to the public domain.
> By submitting a pull request or filing a bug, issue, or
> feature request, you are agreeing to comply with this waiver of copyright interest.
> Details can be found in our [TERMS](TERMS.md) and [LICENSE](LICENSE).


There are two primary ways to help:
- Using the issue tracker, and
- Changing the code-base.


# Using the Issue Tracker

Use the issue tracker to suggest feature requests, report bugs, and ask questions.
This is also a great way to connect with the developers of the project as well
as others who are interested in this solution.

Use the issue tracker to find ways to contribute. Find a bug or a feature, mention in
the issue that you will take on that effort, then follow the _Changing the code-base_
guidance below.


# Changing the Code-Base

* [Summary](#summary)
* [Getting Started](#getting-started)
* [Developing Changes](#developing-changes)
* [Keeping Forks Up to Date](#keeping-forks-up-to-date)

Note that this document only discusses aspects of Git usage that should be needed for day-to-day development and contributions.  For a more detailed overview of ngen's use of Git, see the [GIT_USAGE](doc/GIT_USAGE.md) doc.

## Summary

To work with the repo and contribute changes, the basic process is as follows:

- Create your own fork in Github
- Clone the repo locally (conventionally from your fork) and [setup your repo on your local development machine](#getting-started)
- Make sure to [keep your fork and your local clone(s) up to date](#keeping-forks-up-to-date) with the official OWP ngen repo, ensuring histories remain consistent by performing [rebasing](#rebasing-development-branches)
- Create feature/fix branches from `master` when you want to contribute
- Write changes you want to contribute, commit to your local feature/fix branch, and push these commits to a branch in your personal Github fork
- Submit pull requests to the official OWP repo's `master` branch from a feature/fix branch your fork when the latter branch has a collection of changes ready to be incorporated

## Getting Started

In order to be able to contribute code changes, you will first need to [create a Github fork](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/working-with-forks/fork-a-repo) of the official OWP repo.

Next, set up your authentication  mechanism with Github for your command line (or IDE).  You can either [create an SSH key pair](https://docs.github.com/en/authentication/connecting-to-github-with-ssh/generating-a-new-ssh-key-and-adding-it-to-the-ssh-agent) and [add the public key](https://docs.github.com/en/authentication/connecting-to-github-with-ssh/adding-a-new-ssh-key-to-your-github-account) to your Github account, or you can set up a [Personal Access Token](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/managing-your-personal-access-tokens#using-a-personal-access-token-on-the-command-line) if you plan to clone the repo locally via HTTPS.

After that, [clone a local development repo](https://docs.github.com/en/repositories/creating-and-managing-repositories/cloning-a-repository) from your fork, using a command similar to one of the following:

    # SSH-based clone command.  Change URL to match your fork as appropriate
    git clone git@github.com:your_user/ngen.git

    # HTTPS-based clone command.  Change URL to match your fork as appropriate
    git clone https://github.com/your_user/ngen.git

You can now change directories into the local repo, which will have the _default_ branch - `master` for this repository - checked out.

    # Move into the repo directory "ngen"
    cd ngen

    # You can verify the branch by examining the output of ...
    git status

> [!IMPORTANT]  
> Git will add a [Git remote](https://git-scm.com/book/en/v2/Git-Basics-Working-with-Remotes) named `origin` to the clone's Git configuration that points to the cloned-from repo.  Because of this, the recommended convention is to clone your local repo(s) from your personal fork, thus making `origin` point to your fork.  This is assumed to be the case in other parts of the documentation.

Next, add the OWP ngen repo as a second [Git remote](https://git-scm.com/book/en/v2/Git-Basics-Working-with-Remotes) for the local clone.  Doing the addition will look something like:

    # Add the remote, here using the HTTPS URL (the SSH URL would be fine also)
    git remote add upstream https://github.com/NOAA-OWP/ngen.git

    # Verify
    git remote -v

> [!IMPORTANT]  
> The standard convention used in this doc and elsewhere is to name the Git remote for the official Github OWP repository `upstream`.  In regular text, that Git remote will always be denoted like this.
> 
> Git also has the more general concept of an "upstream branch" associated with "tracking branches", as discussed on the [Git Remote Branches](https://git-scm.com/book/en/v2/Git-Branching-Remote-Branches) documentation.  We will use "upstream branch" when discussing these.

Now set up the user and email in the local repo's configuration.

    git config user.name "John Doe"
    git config user.email "john@doe.org"

Alternatively, one could also set these in the machine's global Git config (or rely upon the global settings if already configured).

     git config --global user.name "John Doe"
     git config --global user.email "john@doe.org"

### Optional: Git Hooks

While optional, Git hooks are a useful feature for helping maintain code quality in your local development repo.  See the repo's [_Git Usage_](doc/GIT_USAGE.md#optional-setting-up-hook-scripts) document for more discussion on these.

[//]: # (TODO: add section/document on code style)


## Developing Changes

* [Work in a Dedicated Branch](#work-in-a-dedicated-branch)
* [Pushing Incremental Commits](#pushing-incremental-commits)
* [Submitting Pull Requests](#submitting-pull-requests)
  * [Guidelines for Pull Requests](#guidelines-for-pull-requests)

### Work in a Dedicated Branch

When you want to contribute a fix or new feature, start by creating and checking out a local branch (e.g., `new_branch`) to contain your work.  This should be based on `master` (you may need to [sync remote changes](#getting-remote-changes) first):

    # Create the new branch "new_branch" based on "master"
    git branch new_branch master

    # Check out "new_branch" locally to work in it
    git checkout new_branch

Go ahead and push this new branch to your fork so it exists there as well.  Use `-u` so that the local branch adds a tracking reference to the branch in your fork:

    # Assuming the convention of your fork's remote being `origin`
    git push -u origin new_branch

> [!IMPORTANT]  
> While `-u` isn't strictly required, including it causes `origin/new_branch` to be set as the "upstream branch" for the local `new_branch` (with the latter being referred to as a "tracking branch").  Tracking branch relationships are described in the _Tracking Branches_ section of [Git's Remote Branches doc](https://git-scm.com/book/en/v2/Git-Branching-Remote-Branches).  
> 
> While it is totally optional, parts our documentation - especially example commands - may assume this tracking branch relationship has been set.

From there begin writing and committing your changes to the branch.  While up to you, it is suggested that development work be committed frequently when changes are complete and meaningful. If work requires modifying more than one file in the source, it is recommended to commit the changes independently to help avoid too large of conflicts if/when they occur.

### Pushing Incremental Commits

Especially if making more frequent, smaller commits as suggested above, it is a good practice to regularly push these smaller commits to your fork.  If the `-u` option was used when initially pushing the branch, it is simple to check if there are local, unpushed commits.

    # The fetch is probably unnecesssary unless you work from multiple local repos
    git fetch
    
    # Assuming your branch of interest is still checked out:
    git status

    # And if there are some newer, local changes that haven't been push yet:
    git push

### Submitting Pull Requests

Once a code contribution is finished, make sure all changes have been pushed to the branch in your fork.  Then you can navigate to the OWP repo via Github's web interface and [submit a PR](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/proposing-changes-to-your-work-with-pull-requests/creating-a-pull-request) to pull this branch into the OWP `master` branch.  Verify that `master` is selected as the recipient branch.  You will also need to make sure you are comparing across forks, choosing your appropriate fork and branch to pull from.  Complete details on the process can be found in Github's documentation.

[//]: # (TODO: consider moving this to its own doc and expanding on details for PRs there some)

#### Guidelines for Pull Requests

The following guidelines are recommended for pull requests. Following these increases the likelihood of a PR being reviewed quickly and minimizes the amount of changes likely to be needed before approval:

* Make sure the PR has an informative and human-readable title
* Limit the scope of changes within a PR to a single goal (no scope creep)
* Ensure the resulting code can be automatically rebased into `master` without conflicts
* Make sure project coding standards, if/when defined, are followed
* Ensure resulting code passes all existing automated tests 
* Ensure any changes or additions to functionality are tested
* Document new functions with a description, list of inputs, and expected output
* Flag any placeholder code is flagged and note future TODO items in comments
* Update repo-level documentation appropriately
* Select one or more appropriate reviewers when creating the PR

#### PR Review and Requested Revisions

Once the PR is submitted, it will be reviewed by one or more other repo contributors.  Often conversations will be had within the Github PR if reviewers have questions or request revisions be made to the proposed changes.  If revisions are requested, you will need to make those in your locally copy of the feature/fix branch, and then re-push that branch (and the updates) to your personal fork.  Then, use the PR page in Github to re-request review.

## Keeping Forks Up to Date

- [A Rebase Strategy](#a-rebase-strategy)
- [Getting Remote Changes](#getting-Remote-changes)
- [Rebasing Development Branches](#rebasing-development-branches)
- [Fixing Diverging Development Branches](#fixing-diverging-development-branches)

### A Rebase Strategy

Development for this repo uses a *rebase* strategy for integrating code changes, rather than a *merge* strategy.  More information on rebasing is [available here](https://git-scm.com/book/en/v2/Git-Branching-Rebasing), but the main takeaway is that it is important all changes are integrated into branches using this approach.  Deviation from this will likely cause a bit of mess with branch commit histories, which could force rejection of otherwise-good PRs.

### Getting Remote Changes

When it is time to check for or apply updates from the official OWP repo to a personal fork and/or a local repo, check out the `master` branch locally  and do fetch-and-rebase, which can be done with `pull` and the `--rebase` option:

    # Checkout local master branch
    git checkout master

    # Fetch and rebase changes
    git pull --rebase upstream master

Then, make sure these get pushed to your personal fork. Assuming [the above-described setup](#getting-started-with-your-fork) where the local repo was cloned from the fork, and assuming the local `master` branch is currently checked out, the command for that is just:

    # Note the assumptions mentioned above that are required for this syntax
    git push

Alternatively, you can use the more explicit form:

`git push <fork_remote> <local_branch>:<remote_branch>`

The previous example command is effectively equivalent to running:

    # Cloning a repo from a fork created a remote for the fork named "origin"; see above assumptions
    git push origin master:master

You also can omit `<local_branch>:` (including the colon) and supply just the remote branch name if the appropriate local branch is still checked out.

#### For `production` Too

Note that the above steps to get remote changes from the official OWP repo can be applied to the `production` branch also (just swap `production` in place of `master`).  `production` should not be used as the basis for feature/fix branches, but there are other reasons why one might want the latest `production` locally or in a personal fork.

### Rebasing Development Branches

When the steps in [Getting Remote Changes](#getting-remote-changes) do bring in new commits that update `master`, it is usually a good idea (and often necessary) to rebase any local feature/fix branches were previously created. E.g.,

    # If using a development branch named 'faster_dataset_writes'
    git checkout faster_dataset_writes
    git rebase master

See documentation on [the "git rebase" command](https://git-scm.com/docs/git-rebase) for more details.

#### Interactive Rebasing

It is possible to have more control over rebasing by doing an interactive rebase.  E.g.:

    git rebase -i master

This will open up a text editor allowing for reordering, squashing, dropping, etc., development branch commits prior to rebasing them onto the new base commit from `master`.  See the [**Interactive Mode**](https://git-scm.com/docs/git-rebase#_interactive_mode) section on the rebase command  for more details.

### Fixing Diverging Development Branches

If a local feature/fix branch is already pushed to a remote fork, and then later rebasing the local branch is necessary, doing so will cause the histories to diverge.  For simple cases, the fix is to just force-push the rebased local branch.

    # To force-push to fix a divergent branch
    git push -f origin feature_branch

However, extra care is needed if multiple developers may be using the branch in the fork (e.g., a developer is collaborating with someone else on a large set of changes for some new feature).  The particular considerations and best ways to go about things in such cases are outside the scope of this document.  Consult Git's documentation and Google, or contact another contributor for advice.
