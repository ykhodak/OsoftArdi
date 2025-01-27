#GVZ_CODE = ../$$(GVZ_CODE)
#GVZ_INSTALL_PATH = ../$$(GVZ_INSTALL_PATH)
GVZ_CODE = ../../graphviz-2.34.0
GVZ_INSTALL_PATH = ../../GV-2.34
#GVZ_CODE = ../graphviz-2.34.0
#GVZ_INSTALL_PATH = ../GV-2.34



ARD_LOCAL_BUILD = $$(ARD_LOCAL_BUILD)

if(isEmpty( GVZ_CODE )){
  error( "GVZ_CODE is not defined. You might need to source 'gvz.envs' first." )
}

if(isEmpty( GVZ_INSTALL_PATH )){
  error( "GVZ_INSTALL_PATH is not defined. You might need to source 'gvz.envs' first." )
}

CDT_DIR  	    = $${GVZ_CODE}/lib/cdt
PATHPLAN_DIR 	    = $${GVZ_CODE}/lib/pathplan
XDOT_DIR 	    = $${GVZ_CODE}/lib/xdot
CGRAPH_DIR 	    = $${GVZ_CODE}/lib/cgraph
COMMON_DIR   	    = $${GVZ_CODE}/lib/common
GVC_DIR   	    = $${GVZ_CODE}/lib/gvc
FDPGEN_DIR	    = $${GVZ_CODE}/lib/fdpgen
LABEL_DIR	    = $${GVZ_CODE}/lib/label
PACK_DIR	    = $${GVZ_CODE}/lib/pack
ORTHO_DIR	    = $${GVZ_CODE}/lib/ortho
SPARSE_DIR	    = $${GVZ_CODE}/lib/sparse
NEATOGEN_DIR	    = $${GVZ_CODE}/lib/neatogen
TWOPIGEN_DIR	    = $${GVZ_CODE}/lib/twopigen
PATCHWORK_DIR	    = $${GVZ_CODE}/lib/patchwork
OSAGE_DIR	    = $${GVZ_CODE}/lib/osage
RBTREE_DIR	    = $${GVZ_CODE}/lib/rbtree
CIRCOGEN_DIR	    = $${GVZ_CODE}/lib/circogen
SFDPGEN_DIR	    = $${GVZ_CODE}/lib/sfdpgen
DOTGEN_DIR	    = $${GVZ_CODE}/lib/dotgen
PLUGIN_CORE_DIR	    = $${GVZ_CODE}/plugin/core
PLUGIN_DOT_LAYOUT_DIR= $${GVZ_CODE}/plugin/dot_layout
PLUGIN_NEATO_LAYOUT_DIR= $${GVZ_CODE}/plugin/neato_layout

INCLUDEPATH 	    += $${GVZ_CODE} $${COMMON_DIR} $${CGRAPH_DIR} $${PATHPLAN_DIR} $${CDT_DIR} $${PACK_DIR} $${XDOT_DIR}
DEFINES 	    += HAVE_CONFIG_H
DEFINES 	    += GVLIBDIR=\\\"graphviz\\\"

DESTDIR = ../lib
OBJECTS_DIR = __tmp
MOC_DIR = __tmp

CONFIG += staticlib
TEMPLATE = lib

win32 {
      DEFINES += YY_NO_UNISTD_H
      DEFINES +=_LIB
      DEFINES += CGRAPH_EXPORTS
      DEFINES += WIN32_DLL
      DEFINES += GVC_EXPORTS
      DEFINES += GVPLUGIN_CORE_EXPORTS
      DEFINES += GVPLUGIN_DOT_LAYOUT_EXPORTS
      DEFINES += GVPLUGIN_NEATO_LAYOUT_EXPORTS
      DEFINES += PATHPLAN_EXPORTS
#      DEFINES += _BLD_cdt
#      DEFINES += GVDLL
#      DEFINES += __EXPORT__
      QMAKE_CXXFLAGS +=  /wd"4290;4996;4100;4244;4018;4189;4305;4101"
}

unix {
     QMAKE_CXXFLAGS += -fPIC
}

if(!isEmpty( ARD_LOCAL_BUILD )){
	  CONFIG += debug
	  CONFIG -= release

    DEFINES += _DEBUG
    unix {
         DEFINES += _SQL_PROFILER
         QMAKE_CXXFLAGS += -O0 -rdynamic
	 QMAKE_LFLAGS += -g -O0 -rdynamic
    }
    win32 {
    	 QMAKE_CXXFLAGS += /Od
    }
}

if(isEmpty( ARD_LOCAL_BUILD )){
	  CONFIG -= debug
	  CONFIG += release
#@todo: comment out this (DEBUG) for stable release version
#	  QMAKE_LFLAGS += /DEBUG
}



CDT_SOURCES = $${CDT_DIR}/dtclose.c $${CDT_DIR}/dtdisc.c $${CDT_DIR}/dtextract.c $${CDT_DIR}/dtflatten.c \
	   $${CDT_DIR}/dthash.c $${CDT_DIR}/dtlist.c $${CDT_DIR}/dtmethod.c $${CDT_DIR}/dtopen.c $${CDT_DIR}/dtrenew.c $${CDT_DIR}/dtrestore.c $${CDT_DIR}/dtsize.c \
	   $${CDT_DIR}/dtstat.c $${CDT_DIR}/dtstrhash.c $${CDT_DIR}/dttree.c $${CDT_DIR}/dttreeset.c $${CDT_DIR}/dtview.c $${CDT_DIR}/dtwalk.c

PATHPLAN_SOURCES = $${PATHPLAN_DIR}/cvt.c $${PATHPLAN_DIR}/inpoly.c $${PATHPLAN_DIR}/route.c $${PATHPLAN_DIR}/shortest.c \
		 $${PATHPLAN_DIR}/shortestpth.c $${PATHPLAN_DIR}/solvers.c $${PATHPLAN_DIR}/triang.c $${PATHPLAN_DIR}/util.c $${PATHPLAN_DIR}/visibility.c


XDOT_SOURCES = $${XDOT_DIR}/xdot.c

CGRAPH_SOURCES = $${CGRAPH_DIR}/agerror.c $${CGRAPH_DIR}/agxbuf.c $${CGRAPH_DIR}/apply.c $${CGRAPH_DIR}/attr.c $${CGRAPH_DIR}/edge.c \
	   $${CGRAPH_DIR}/flatten.c $${CGRAPH_DIR}/graph.c $${CGRAPH_DIR}/grammar.c $${CGRAPH_DIR}/id.c $${CGRAPH_DIR}/imap.c $${CGRAPH_DIR}/io.c $${CGRAPH_DIR}/mem.c $${CGRAPH_DIR}/node.c \
	   $${CGRAPH_DIR}/obj.c $${CGRAPH_DIR}/pend.c $${CGRAPH_DIR}/rec.c $${CGRAPH_DIR}/refstr.c $${CGRAPH_DIR}/scan.c $${CGRAPH_DIR}/subg.c $${CGRAPH_DIR}/utils.c $${CGRAPH_DIR}/write.c


COMMON_SOURCES = $${COMMON_DIR}/arrows.c $${COMMON_DIR}/colxlate.c $${COMMON_DIR}/ellipse.c $${COMMON_DIR}/fontmetrics.c \
	     $${COMMON_DIR}/args.c $${COMMON_DIR}/memory.c $${COMMON_DIR}/globals.c $${COMMON_DIR}/htmllex.c $${COMMON_DIR}/htmlparse.c $${COMMON_DIR}/htmltable.c $${COMMON_DIR}/input.c\
	     $${COMMON_DIR}/pointset.c $${COMMON_DIR}/intset.c $${COMMON_DIR}/postproc.c $${COMMON_DIR}/routespl.c $${COMMON_DIR}/splines.c $${COMMON_DIR}/psusershape.c\
	     $${COMMON_DIR}/timing.c $${COMMON_DIR}/labels.c $${COMMON_DIR}/ns.c $${COMMON_DIR}/shapes.c $${COMMON_DIR}/utils.c $${COMMON_DIR}/geom.c $${COMMON_DIR}/taper.c\
	     $${COMMON_DIR}/output.c $${COMMON_DIR}/emit.c


GVC_SOURCES = $${GVC_DIR}/gvrender.c $${GVC_DIR}/gvlayout.c $${GVC_DIR}/gvdevice.c $${GVC_DIR}/gvloadimage.c \
	    $${GVC_DIR}/gvcontext.c  $${GVC_DIR}/gvevent.c  $${GVC_DIR}/gvconfig.c \
	    $${GVC_DIR}/gvtextlayout.c $${GVC_DIR}/gvusershape.c \
	    $${GVC_DIR}/gvc.c $${GVC_DIR}/gvplugin.c $${GVC_DIR}/gvjobs.c


LABEL_SOURCES = $${LABEL_DIR}/xlabels.c $${LABEL_DIR}/index.c $${LABEL_DIR}/node.c $${LABEL_DIR}/rectangle.c $${LABEL_DIR}/split.q.c

PACK_SOURCES = $${PACK_DIR}/ccomps.c $${PACK_DIR}/pack.c


ORTHO_SOURCES = $${ORTHO_DIR}/fPQ.c $${ORTHO_DIR}/maze.c $${ORTHO_DIR}/ortho.c $${ORTHO_DIR}/partition.c $${ORTHO_DIR}/rawgraph.c $${ORTHO_DIR}/sgraph.c $${ORTHO_DIR}/trapezoid.c


NEATOGEN_SOURCES = $${NEATOGEN_DIR}/adjust.c $${NEATOGEN_DIR}/circuit.c $${NEATOGEN_DIR}/edges.c $${NEATOGEN_DIR}/geometry.c \
	$${NEATOGEN_DIR}/heap.c $${NEATOGEN_DIR}/hedges.c $${NEATOGEN_DIR}/info.c $${NEATOGEN_DIR}/neatoinit.c $${NEATOGEN_DIR}/legal.c $${NEATOGEN_DIR}/lu.c $${NEATOGEN_DIR}/matinv.c \
	$${NEATOGEN_DIR}/memory.c $${NEATOGEN_DIR}/poly.c $${NEATOGEN_DIR}/printvis.c $${NEATOGEN_DIR}/site.c $${NEATOGEN_DIR}/solve.c $${NEATOGEN_DIR}/neatosplines.c $${NEATOGEN_DIR}/stuff.c \
	$${NEATOGEN_DIR}/voronoi.c $${NEATOGEN_DIR}/stress.c $${NEATOGEN_DIR}/kkutils.c $${NEATOGEN_DIR}/matrix_ops.c $${NEATOGEN_DIR}/embed_graph.c $${NEATOGEN_DIR}/dijkstra.c \
	$${NEATOGEN_DIR}/conjgrad.c $${NEATOGEN_DIR}/pca.c $${NEATOGEN_DIR}/closest.c $${NEATOGEN_DIR}/bfs.c $${NEATOGEN_DIR}/constraint.c $${NEATOGEN_DIR}/quad_prog_solve.c \
	$${NEATOGEN_DIR}/smart_ini_x.c $${NEATOGEN_DIR}/constrained_majorization.c $${NEATOGEN_DIR}/opt_arrangement.c \
	$${NEATOGEN_DIR}/overlap.c $${NEATOGEN_DIR}/call_tri.c \
	$${NEATOGEN_DIR}/compute_hierarchy.c $${NEATOGEN_DIR}/delaunay.c $${NEATOGEN_DIR}/multispline.c


SPARSE_SOURCES = $${SPARSE_DIR}/SparseMatrix.c $${SPARSE_DIR}/general.c $${SPARSE_DIR}/BinaryHeap.c $${SPARSE_DIR}/IntStack.c $${SPARSE_DIR}/vector.c $${SPARSE_DIR}/DotIO.c \
    $${SPARSE_DIR}/LinkedList.c $${SPARSE_DIR}/colorutil.c $${SPARSE_DIR}/mq.c $${SPARSE_DIR}/clustering.c

TWOPIGEN_SOURCES = $${TWOPIGEN_DIR}/twopiinit.c $${TWOPIGEN_DIR}/circle.c


PATCHWORK_SOURCES = $${PATCHWORK_DIR}/patchwork.c $${PATCHWORK_DIR}/patchworkinit.c $${PATCHWORK_DIR}/tree_map.c


OSAGE_SOURCES = $${OSAGE_DIR}/osageinit.c

FDPGEN_SOURCES = $${FDPGEN_DIR}/comp.c $${FDPGEN_DIR}/dbg.c $${FDPGEN_DIR}/grid.c $${FDPGEN_DIR}/fdpinit.c $${FDPGEN_DIR}/layout.c \
	$${FDPGEN_DIR}/tlayout.c $${FDPGEN_DIR}/xlayout.c $${FDPGEN_DIR}/clusteredges.c


RBTREE_SOURCES = $${RBTREE_DIR}/misc.c $${RBTREE_DIR}/red_black_tree.c $${RBTREE_DIR}/stack.c

CIRCOGEN_SOURCES = $${CIRCOGEN_DIR}/circularinit.c $${CIRCOGEN_DIR}/nodelist.c $${CIRCOGEN_DIR}/block.c $${CIRCOGEN_DIR}/edgelist.c \
	$${CIRCOGEN_DIR}/circular.c $${CIRCOGEN_DIR}/deglist.c $${CIRCOGEN_DIR}/blocktree.c $${CIRCOGEN_DIR}/blockpath.c \
	$${CIRCOGEN_DIR}/circpos.c $${CIRCOGEN_DIR}/nodeset.c

SFDPGEN_SOURCES = $${SFDPGEN_DIR}/sfdpinit.c $${SFDPGEN_DIR}/spring_electrical.c \
	$${SFDPGEN_DIR}/sparse_solve.c $${SFDPGEN_DIR}/post_process.c \
	$${SFDPGEN_DIR}/stress_model.c $${SFDPGEN_DIR}/uniform_stress.c \
	$${SFDPGEN_DIR}/QuadTree.c $${SFDPGEN_DIR}/Multilevel.c $${SFDPGEN_DIR}/PriorityQueue.c


DOTGEN_SOURCES = $${DOTGEN_DIR}/acyclic.c $${DOTGEN_DIR}/class1.c $${DOTGEN_DIR}/class2.c $${DOTGEN_DIR}/cluster.c $${DOTGEN_DIR}/compound.c \
	$${DOTGEN_DIR}/conc.c $${DOTGEN_DIR}/decomp.c $${DOTGEN_DIR}/fastgr.c $${DOTGEN_DIR}/flat.c $${DOTGEN_DIR}/dotinit.c $${DOTGEN_DIR}/mincross.c \
	$${DOTGEN_DIR}/position.c $${DOTGEN_DIR}/rank.c $${DOTGEN_DIR}/sameport.c $${DOTGEN_DIR}/dotsplines.c $${DOTGEN_DIR}/aspect.c


PLUGIN_CORE_SOURCES = $${PLUGIN_CORE_DIR}/gvplugin_core.c \
	$${PLUGIN_CORE_DIR}/gvrender_core_dot.c \
	$${PLUGIN_CORE_DIR}/gvrender_core_fig.c \
	$${PLUGIN_CORE_DIR}/gvrender_core_map.c \
	$${PLUGIN_CORE_DIR}/gvrender_core_ps.c \
	$${PLUGIN_CORE_DIR}/gvrender_core_svg.c \
	$${PLUGIN_CORE_DIR}/gvrender_core_tk.c \
	$${PLUGIN_CORE_DIR}/gvrender_core_vml.c \
	$${PLUGIN_CORE_DIR}/gvrender_core_pov.c \
	$${PLUGIN_CORE_DIR}/gvrender_core_pic.c \
	$${PLUGIN_CORE_DIR}/gvloadimage_core.c 


PLUGIN_DOT_LAYOUT_SOURCES = $${PLUGIN_DOT_LAYOUT_DIR}/gvplugin_dot_layout.c \
			  $${PLUGIN_DOT_LAYOUT_DIR}/gvlayout_dot_layout.c


PLUGIN_NEATO_LAYOUT_SOURCES = $${PLUGIN_NEATO_LAYOUT_DIR}/gvplugin_neato_layout.c \
			    $${PLUGIN_NEATO_LAYOUT_DIR}/gvlayout_neato_layout.c


### deprecated ###
#
#GRAPHVIZ_STATIC_INCLUDE = $${COMMON_DIR} $${CGRAPH_DIR} $${CDT_DIR} $${GVC_DIR} $${XDOT_DIR} $${PACK_DIR}  $${ORTHO_DIR} $${LABEL_DIR} $${PATHPLAN_DIR} $${DOTGEN_DIR} $${SFDPGEN_DIR} $${FDPGEN_DIR} $${TWOPIGEN_DIR} $${NEATOGEN_DIR} $${CIRCOGEN_DIR} $${OSAGE_DIR} $${SPARSE_DIR} $${PATCHWORK_DIR} $${PLUGIN_CORE_DIR} $${PLUGIN_DOT_LAYOUT_DIR} $${PLUGIN_NEATO_LAYOUT_DIR}
#
#GRAPHVIZ_STATIC_SOURCES = $${COMMON_SOURCES} $${CGRAPH_SOURCES} $${CDT_SOURCES} $${GVC_SOURCES} $${XDOT_SOURCES} $${PACK_SOURCES}  $${ORTHO_SOURCES} $${LABEL_SOURCES} $${PATHPLAN_SOURCES} $${DOTGEN_SOURCES} $${SFDPGEN_SOURCES} $${FDPGEN_SOURCES} $${TWOPIGEN_SOURCES} $${NEATOGEN_SOURCES} $${CIRCOGEN_SOURCES} $${OSAGE_SOURCES} $${SPARSE_SOURCES} $${PATCHWORK_SOURCES} $${PLUGIN_CORE_SOURCES} $${PLUGIN_DOT_LAYOUT_SOURCES} $${PLUGIN_NEATO_LAYOUT_SOURCES} 
#
#



