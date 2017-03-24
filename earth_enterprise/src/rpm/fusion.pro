%defattr(555,root,root)

%{gebindir}/gerasterimport
%{gebindir}/gevirtualraster
%{gebindir}/gereproject
%{gebindir}/gemaskgen
%{gebindir}/geinfo
%{gebindir}/gepackgen
%{gebindir}/gesplitkhvr
%{gebindir}/gerasterdbrootgen
%{gebindir}/gecombineterrain
%{gebindir}/gerasterpackupgrade

# autoingest stuff
%defattr(555,root,root)
%{gebindir}/genewimageryresource
%{gebindir}/genewterrainresource
%{gebindir}/gemodifyimageryresource
%{gebindir}/gemodifyterrainresource
%{gebindir}/genewimageryproject
%{gebindir}/genewterrainproject
%{gebindir}/gemodifyimageryproject
%{gebindir}/gemodifyterrainproject
%{gebindir}/geaddtoimageryproject
%{gebindir}/geaddtoterrainproject
%{gebindir}/gedropfromimageryproject
%{gebindir}/gedropfromterrainproject

# additional tools

%{gebindir}/getranslate


# ###########################
# for backwards compatibility
# ###########################
%defattr(-,root,root)
%{gebindir}/khaddtoimageryproject
%{gebindir}/khaddtoterrainproject
%{gebindir}/khdropfromimageryproject
%{gebindir}/khdropfromterrainproject
%{gebindir}/khmodifyimageryproject
%{gebindir}/khmodifyterrainproject
%{gebindir}/khnewimageryproject
%{gebindir}/khnewterrainproject
%{gebindir}/khnewimageryasset
%{gebindir}/khnewterrainasset
%{gebindir}/khmodifyimageryasset
%{gebindir}/khmodifyterrainasset
%{gebindir}/khmaskgen
%{gebindir}/khvirtualraster
%{gebindir}/khtranslate
%{gebindir}/khreproject
%{gebindir}/khupdatelut
