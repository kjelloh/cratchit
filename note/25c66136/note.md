# Git merge with commit from branch to master

According to chartGPT the following should do the trick:

```sh
# 1. Switch to master
git checkout master

# 2. Make sure it's up to date (optional but recommended)
git pull origin master

# 3. Merge your branch with a merge commit
git merge --no-ff clean-up-sie-posted-staged-update-mechanism

# 4. Push the result
git push origin master
```

Also, I need to first stage and commit all changes on the branch!
