function util = compute_utility(qn, scale)
    util = sum(log(qn*scale));
end