# Publishing

The repository is local; remote publication is pending authenticated repository creation. Create an empty `smart-parking-barrier` repository in the intended GitHub account, then add its actual URL as origin and push main. Do not assume a URL exists. Use the account's normal Git authentication; never commit credentials.

```sh
git remote add origin YOUR_ACTUAL_REPOSITORY_URL
git push -u origin main
```

Check GitHub Actions, README diagrams and links. Keep the physical-validation status visible until real results are available. Add actual photos, a publicly viewable demo link and acceptance records, then verify the repository and video while signed out. `.tools/`, `build/` and `dist/` are intentionally excluded; the downloadable source archive is generated locally by `python tools/package_project.py`.
