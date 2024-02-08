# Github Automated Testing

## What Is Automated Test and How to Start It

In **ngen** github repo, we use github actions/workflows to automatically test the validity of commited codes by developers. The test is triggered when a developer pushes some codes to a branch in his fork and creates a `Pull Request`. The successful test is marked by a green check mark, a failed test is marked by a red cross mark. If a test fails, you have to debug your codes (see [What to Do When a Test Fail?](#what-to-do-when-a-test-fail?) below) and commit and push to the same branch again and the automatic testing will restart in the `Pull Request`. 

## What Code Tests Are Performed

- Unit tests: this inclodes every set of codes that serves a unique functionality. Unit test eveolves as new codes are added.
- BMI (Basical Model Interface) formulation interface tests including codes in C, C++, Fortran, and Python.
- Running **ngen** executable on example hydrofabric with various realistic modules/models, initial condition, and forcing data.

## What to Do When a Test Fail?

- Before getting into debugging, first thing, we recommend that you perform all the necessary tests listed above on you local computer. If all tests are successful, then push up your codes to the intended branch in your fork.
- Sometimes, some tests may fail even they have passed tests on local computer. If that happens, you have to look into details why they failed. To do that, click on the word `Details` in blue on the right for a particular test. This will open a window with detailed information for that particular test, the error information are usually at the bottom. You can scroll up and down the side bar for more information. you can also search for key workds in *Search logs* at the upper right corner.
- Othertimes, if you are lucky, the test may have failed due to time out, in that case, you may rerun the test by pointing the cursor on the test name, a cycling icon will appear and you can rerun your test by clicking on the icon. In any case, you may manually rerun any failed test by following procedure described above. But it is recommended taht you carefully study the fail error first.
