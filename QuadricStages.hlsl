//runs per quadric
[numthreads(32, 1, 1)]
void Projection( uint3 DTid : SV_DispatchThreadID )
{
    //get input quadric
    
    //transform to viewSpace
    
    //find intersection with silhouette (polar) plane and quadric
    
    //X MIN&MAX
    //find the two planes,
        //perpendicular to polar plane (parallel to CAMERA-FORWARD // polar plane normal)
        //normal of these planes is parallel to CAMERA-RIGHT (so is equal to) == (WRONG)
        //with 1 intersection point w the Quadric
        //transform intersection points to projection space to find bounding box max&min X
        
    //Y MIN&MAX
    //find the two planes,
        //perpendicular to polar plane (parallel to CAMERA-FORWARD // polar plane normal)
        //normal of these planes is parallel to CAMERA-UP (so is equal to) == (WRONG)
        //with 1 intersection point w the Quadric
        //transform intersection points to projection space to find bounding box max&min Y
    
    //save these points in the output quadric as patchCoordinates
    //patch : 8x8 pixels that will be shaded in parallel in a thread group
    
    //outputQuadric : 
        //quadric in ViewSpace
        //startPatch X , Y
        //numPatches X , Y

    //outputMesh :
        //numPatches += X * Y; (interlockedAdd)
}


//runs per patch (8x8 pixels : 64 threads per group)
[numthreads(8, 8, 1)]
void Rasterization(uint3 DTid : SV_DispatchThreadID)
{
    //(groupShared)
    //get patch to shade (DTid.z)
    //find outputquadric linked to this patch
    
    //**********
    //(per thread)
    //cast ray starting from DTid.xy and find depth of the quadric at the pixel
    //if HIT
    //FIND NORMAL
    //SHADE
}
