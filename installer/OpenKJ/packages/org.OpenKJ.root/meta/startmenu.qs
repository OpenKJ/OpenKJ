function Component()
{
    // default constructor
}

Component.prototype.createOperations = function()
{
    // call default implementation to actually install README.txt!
    component.createOperations();

    if (systemInfo.productType === "windows") {
        component.addOperation("CreateShortcut", "@TargetDir@/KaraokeHost.exe", "@StartMenuDir@/KaraokeHost.lnk",
            "workingDirectory=@TargetDir@");
		component.addOperation("CreateShortcut", "@TargetDir@/BreakMusic.exe", "@StartMenuDir@/BreakMusic.lnk",
            "workingDirectory=@TargetDir@");
    }
}
