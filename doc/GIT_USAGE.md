# Git Strategy

- [Branching Design](#branching-design)
- [Contributing](#contributing)
    - [TLDR;](#contributing-tldr)
    - [Fork Consistency Requirements](#fork-consistency-requirements)
    - [Fork Setup Suggestions](#fork-setup-suggestions)
- [Keeping Forks Up to Date](#keeping-forks-up-to-date)
    - [Setting the upstream Remote](#setting-the-upstream-remote)
    - [Getting Upstream Changes](#getting-upstream-changes)
    - [Rebasing Development Branches](#rebasing-development-branches)
    - [Fixing Diverging Development Branches](#fixing-diverging-development-branches)

## Branching Design

- Main **upstream** repo has only the single long-term branch called **master**; i.e., `upstream/master` 
    - There may occasionally be other temporary branches for specific purposes
- Interaction with **upstream** repo is done exclusively via pull requests (PRs) to `upstream/master`

## Contributing

- [TLDR;](#contributing-tldr)
- [Fork Consistency Requirements](#fork-consistency-requirements)
- [Fork Setup Suggestions](#fork-setup-suggestions)

### Contributing TLDR;

To work with the repo and contribute changes, the basic process is as follows:

- Create your own fork in Github of the main **upstream** repository
- Clone your fork on your local development machine
- Make sure to keep your fork and your local clone(s) up to date with the main upstream repo, keeping histories consistent via rebasing
- In the local repo, push the changes you make to your Github fork
- Submit pull requests to `upstream/master` when you have changes ready to be added

### Fork Consistency Requirements

Within your own local repo and personal fork, you are mostly free to do whatever branching strategy works for you.  However, branches used for PRs must have all new commits based on the current `HEAD` commit of the `upstream/master` branch, to ensure the repo history remains consistent.  I.e., you need to make sure you rebase the branch with your changes before making a PR.  How you go about this is up to you, but the following suggested setup will make that relatively easy.

### Fork Setup Suggestions

After creating your fork in Github, clone your local repo from the fork.  This should make your fork a remote for you local repo, typically named **origin**.  Then,  [add the **upstream** as a second remote](#setting-the-upstream-remote) in your local repo.

Have your own `master` branch, on your local clone and within your personal fork, just as [a place to rebase changes from `upstream/master`](#getting-upstream-changes).  Do not do any development work or add any of your own changes directly.  Just keep it as a "clean," current copy of the `upstream/master` branch.  

Use separate branches for development work as you see fit.  When preparing to make a PR, create a new branch just for that PR, making sure it is both up to date with **upstream** and has all the desired local changes.  Wait to actually push it to your fork until that has been done and your are ready create the PR.

A separate, "clean" local `master` should be easy to keep it in sync with `upstream/master`, which in turn will make it relatively easy to rebase local development branches whenever needed.  This simplifies maintaining the base-commit consistency requirement for the branches you will use for pull requests.

Clean up above mentioned PR branches regularly (i.e., once their changes get incorporated).  You may also want to do this for the other development branches in your fork, or else you'll end up with branches having [diverged histories that need to be fixed](#fixing-diverging-development-branches).

## Keeping Forks Up to Date

- [Setting the upstream Remote](#setting-the-upstream-remote)
- [Getting Upstream Changes](#getting-upstream-changes)
- [Rebasing Development Branches](#rebasing-development-branches)
- [Fixing Diverging Development Branches](#fixing-diverging-development-branches)

### Setting the **upstream** Remote

To stay in sync with other separate changes added to **upstream**, you will typically need to add the main upstream repository as a Git **remote** on your local development machine.  The standard convention, used here and elsewhere, is to name that remote **upstream**.  Doing the addition will look something like:

    # Add the remote 
    git remote add upstream https://github.com/NOAA-OWP/ngen.git
    
    # Verify
    git remote -v

### Getting Upstream Changes

When you want to check for or apply updates to your fork (and your local repo), locally check out your `master` branch and do fetch-and-rebase, which can be done with `pull` and the `--rebase` option:

    # Checkout local master branch 
    git checkout master
    
    # Fetch and rebase changes
    git pull --rebase upstream master
    
Then, make sure these get pushed to your fork. Assuming a typical setup where you have cloned from your fork, and you still have `master` checked out, that is just:

    # Note the assumptions mentioned above that required for this syntax
    git push

Depending on your individual setup, you may want to do this immediately (e.g., if your `master` branch is "clean", as [discussed in the forking suggestions](#fork-setup-suggestions)), or wait until your local `master` is in a state ready to push to your fork. 

### Rebasing Development Branches    
    
When the steps in [Getting Upstream Changes](#getting-upstream-changes) do bring in new commits that update `master`, you should rebase your other local branches. E.g., 

    # If using a development branch named 'dev'
    git checkout dev
    git rebase master
    
Alternatively, you can do an interactive rebase.  This will open up a text editor allowing you to rearrange, squash, omit, etc. your commits when you rebase your development branch onto the new state of `master`. 

    git rebase -i master
    
### Fixing Diverging Development Branches

If you have already pushed a local development branch to your fork, and then later need to rebase the branch, doing so will cause the history to diverge.  If you are the only one using your fork, this is easy to fix by simply force-pushing your rebased local branch.

    # To force-push to fix a divergent branch
    git push -f origin dev
    
However, you will need to be careful with this if you are not the only one using you fork (e.g., you are collaborating with someone else on a large set of changes for some new feature).  The particular considerations and best ways to go about things in such cases are outside the scope of this document.  Consult Git's documentation and Google, or contact another contributor for advice.

     