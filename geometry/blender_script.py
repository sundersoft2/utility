import os
import chunk
import bpy
import mathutils
from mathutils.geometry import tessellate_polygon
from bpy_extras.io_utils import ExportHelper
import time
import math
import struct
import bmesh
from mathutils import Vector
import fnmatch

file="";

def write(targ):
    global file;
    file.write(str(targ));
    file.write("\n");

def write_v2(targ):
    write(targ.x);
    write(targ.y);

def write_v3(targ):
    write_v2(targ);
    write(targ.z);

def write_v4(targ):
    write_v3(targ);
    write(targ.w);


class read_eof(Exception):
    ""
def read_string():
    global file;
    res=file.readline();
    if res=="":
        raise read_eof();
    if res[-1]=="\n":
        res=res[0:-1];
    return res;

def read_int():
    return int(read_string());

def read_float():
    return float(read_string());

def read_v2():
    x=read_float();
    y=read_float();
    return Vector((x, y));

def read_v3():
    x=read_float();
    y=read_float();
    z=read_float();
    return Vector((x, y, z));

def read_v4():
    x=read_float();
    y=read_float();
    z=read_float();
    w=read_float();
    return Vector((x, y, z, w));


def write_scene_object(t_name):
    c_object=bpy.data.objects.get(t_name);
    if c_object==None:
        print("Export - skipped", t_name);
        return;
    print("Exporting", t_name);
    #
    bm=bmesh.new();
    bm.from_mesh(c_object.data);
    #
    write(c_object.name);
    c_object.scale=(1,1,1);
    write_v3(c_object.location);
    assert(c_object.rotation_mode=="QUATERNION");
    write_v4(c_object.rotation_quaternion);
    #
    write(len(bm.verts));
    for i in bm.verts:
        write_v3(i.co);
    #
    write(len(bm.faces));
    for i in bm.faces:
        write(i.material_index);
        write(len(i.verts));
        for j in i.verts:
            write(j.index);
    #
    bm.free();


def read_scene_object():
    t_name=read_string();
    print("Importing", t_name);
    #
    bm=bmesh.new();
    t_location=read_v3();
    t_rotation_v=read_v4();
    t_rotation=Vector((t_rotation_v.w, t_rotation_v.x, t_rotation_v.y, t_rotation_v.z));
    #
    num_verts=read_int();
    for i in range(num_verts):
        v=bm.verts.new(read_v3());
    bm.verts.index_update();
    bm.verts.ensure_lookup_table();
    #
    num_faces=read_int();
    for i in range(num_faces):
        material_index=read_int();
        num_face_verts=read_int();
        face_verts=[];
        for j in range(num_face_verts):
            face_verts.append(bm.verts[read_int()]);
        c_face=bm.faces.new(face_verts);
        c_face.material_index=material_index;
    #
    template_object=bpy.data.objects.get("template");
    c_object=template_object.copy();
    c_object.name=t_name;
    c_object.data=template_object.data.copy();
    bpy.context.scene.objects.link(c_object);
    c_object.layers[1]=True;
    for i in range(20):
        c_object.layers[i]=(i==1);
    #
    #make sure everything got read without errors before assigning these
    bm.to_mesh(c_object.data);
    c_object.location=t_location;
    c_object.rotation_quaternion=t_rotation;
    bm.free();


def do_export():
    global file;
    file=open(bpy.path.abspath("//_blender_export_mesh.txt"), "w");
    try:
        for o in bpy.data.objects:
            if fnmatch.fnmatchcase(o.name, "in*"):
                write_scene_object(o.name);
    finally:
        file.close();

def do_import():
    global file;
    try:
        file=open(bpy.path.abspath("//_blender_import_mesh.txt"), "r");
    except IOError:
        print("Import - File not found");
        return;
    try:
        for o in bpy.data.objects:
            o.select=fnmatch.fnmatchcase(o.name, "out*");
        bpy.ops.object.delete();

        for x in range(0, 10000):
            read_scene_object();
        raise Exception();
    except read_eof:
        print("EOF exception");
    finally:
        file.close();

do_export();
do_import();
