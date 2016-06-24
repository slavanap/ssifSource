-- old depth picking algorithm. Pick the most foregorund object.

function CalcForFrame(framenum)
	local min_depth, max_depth, mean_depth = CalcForFrameRaw();
    local res_depth = -min_depth;
    print(string.format("debug %3d front %3d back %3d mean %3d res %3d",
        framenum, -min_depth, -max_depth, -mean_depth, res_depth));
    return res_depth;
end

function CalcForSub(startframenum, lengthinframes, depths)
    local value = math.max(table.unpack(depths));
    print(string.format("depths: sub started at frame #%d has depth value %d", startframenum, value));
    return value;
end
