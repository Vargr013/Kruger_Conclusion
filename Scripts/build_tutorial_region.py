"""Build only the First Day on Patrol additions in the existing Skekuza map.
Run in UE 5.7 Python with the editor otherwise closed. Refuses a second placement.
"""
import unreal, random, math, json
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
MAP='/Game/Poaching_Patrol/Levels/Skekuza_Level_Final'
TAG='PPTutorial'
editor=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP)
unreal.WorldPartitionBlueprintLibrary.load_actors([d.guid for d in unreal.WorldPartitionBlueprintLibrary.get_actor_descs()])
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
original=list(editor.get_all_level_actors())
if any(a.actor_has_tag(TAG) for a in original): raise RuntimeError('Tutorial already exists; refusing duplicate placement.')
created=[]
random.seed(18092026)
def ground(x,y):
    hit=unreal.SystemLibrary.line_trace_single(world,unreal.Vector(x,y,10000),unreal.Vector(x,y,-10000),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,created,unreal.DrawDebugTrace.NONE,True)
    if not hit: raise RuntimeError('No terrain at {},{}'.format(x,y))
    return hit.to_tuple()[4].z

def label(a,name,folder='Environment'):
    a.set_actor_label('Tutorial_'+name)
    a.set_folder_path('First Day on Patrol/'+folder)
    a.set_editor_property('tags',[unreal.Name(TAG)])
    a.set_editor_property('is_spatially_loaded',False)
    created.append(a)
    return a

def spawn(cls,name,x,y,offset=0,yaw=0,folder='Gameplay'):
    return label(editor.spawn_actor_from_class(cls,unreal.Vector(x,y,ground(x,y)+offset),unreal.Rotator(yaw=yaw)),name,folder)

mesh_cache={}
def mesh(path,name,x,y,height=None,yaw=0,offset=0,collision=True):
    asset=mesh_cache.setdefault(path,unreal.load_asset(path))
    if not asset: raise RuntimeError(path)
    a=spawn(unreal.StaticMeshActor,name,x,y,0,yaw,'Environment')
    comp=a.static_mesh_component
    comp.set_static_mesh(asset)
    box=asset.get_bounding_box()
    scale=height/max(1,box.max.z-box.min.z) if height else 1
    a.set_actor_scale3d(unreal.Vector(scale,scale,scale))
    a.set_actor_location(unreal.Vector(x,y,ground(x,y)-box.min.z*scale+offset),False,False)
    comp.set_collision_profile_name('BlockAll' if collision else 'NoCollision')
    return a

def sign(text,name,x,y,yaw=180):
    a=spawn(unreal.TextRenderActor,name,x,y,230,yaw,'Wayfinding')
    c=a.get_component_by_class(unreal.TextRenderComponent)
    c.set_text(text); c.set_world_size(35); c.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    c.set_text_render_color(unreal.Color(245,205,130,255))
    mesh('/Engine/BasicShapes/Cube','Post_'+name,x,y,height=240,collision=False).set_actor_scale3d(unreal.Vector(.12,.12,2.4))
    return a

# Native configuration and assignments. Original PlayerStart stays untouched.
director=spawn(unreal.PPTutorialDirector,'Director',104500,18500)
start=spawn(unreal.TargetPoint,'Start',103400,19800,105,-45)
brief=spawn(unreal.TargetPoint,'Briefing',104600,18600,100)
destination=spawn(unreal.TargetPoint,'EncounterDestination',118000,30900,90)
rest_cls=unreal.EditorAssetLibrary.load_blueprint_class('/Game/Poaching_Patrol/Assets/BP_LandCruiserRestPoint')
rest=spawn(rest_cls,'Resupply',105600,19500,75,90)
practice=spawn(unreal.PPTutorialTarget,'PracticeTarget',106200,21300,150,0)
practice.set_actor_rotation(unreal.Rotator(pitch=90,yaw=90),False)
poacher_cls=unreal.EditorAssetLibrary.load_blueprint_class('/Game/Poaching_Patrol/Characters/Poachers/MyPPPoacherCharacter_Test')
poacher=spawn(poacher_cls,'Poacher',118000,30900,100,210)
poacher.set_editor_property('tutorial_encounter',True)
poacher.set_editor_property('start_encounter_inactive',True)
zone_cls=unreal.EditorAssetLibrary.load_blueprint_class('/Game/Poaching_Patrol/Assets/MyPPArrestZone')
zone=spawn(zone_cls,'ArrestZone',104500,20400,150)
zone.set_editor_property('zone_extent',unreal.Vector(550,550,250))
for key,value in [('tutorial_start',start),('briefing_point',brief),('patrol_destination',destination),('resupply_point',rest),('practice_target',practice),('tutorial_poacher',poacher),('arrest_zone',zone)]: director.set_editor_property(key,value)
# PatrolMode defaults to Tutorial; change it on the director before starting PIE.

# Reuse one complete tent assembly, preserving its imported mesh pivots and scale.
source=[a for a in original if a.get_actor_label() in ['TentBaze','TentTop','TentWindow']]
for a in source:
    b=editor.duplicate_actor(a,world,unreal.Vector(0,0,0))
    label(b,'Camp_'+a.get_actor_label())
    b.set_actor_location(unreal.Vector(104000,18000,ground(104000,18000)),False,False)
# A simple ranger stand-in stays at the post; character polish comes after testing.
body=mesh('/Engine/BasicShapes/Cylinder','SeniorRanger_StandIn',104600,18400,height=175,collision=True)
body.set_actor_scale3d(unreal.Vector(.6,.6,1.75))
mesh('/Engine/BasicShapes/Sphere','SeniorRanger_Head',104600,18400,height=43,offset=175,collision=False)
sign('SENIOR RANGER','RangerLabel',104600,18400,90)
sign('RANGER POST','CampSign',103800,19600,135)
sign('RESTOCK - HOLD E','SupplySign',105600,19000,90)
sign('PRACTICE TARGET','PracticeSign',106200,21500,270)
sign('ARREST HANDOVER','ArrestSign',104500,20900,270)
mesh('/Game/Foliage/Boabab/Boabab1','CampLandmark',103000,18000,height=1650,yaw=30)

# A marked looping route leaves the camp and divides around the outcrop.
route=[(106500,21700),(109000,23000),(111000,25200),(112000,27300)]
left=[route[-1],(111600,29600),(113800,31400),(116200,31800),(118000,30900)]
right=[route[-1],(114300,26800),(116200,28000),(117200,29900),(118000,30900)]
paths=[[(104500,20400)]+route,left,right]
for i,(x,y) in enumerate(route):
    sign('PATROL TRAIL >','Trail_{}'.format(i),x-220,y-100,225)
sign('CLEARING - EITHER PATH','Choice',112000,27100,225)
sign('< RANGER POST','Return',117500,30200,45)
# Trail edges use small existing rocks, leaving the walking surface untouched.
for k,path in enumerate(paths):
    for j,(p,q) in enumerate(zip(path,path[1:])):
        dx,dy=q[0]-p[0],q[1]-p[1]; length=math.hypot(dx,dy)
        for i in range(max(1,int(length/700))):
            t=i/max(1,int(length/700)); x=p[0]+dx*t; y=p[1]+dy*t
            for side in [-1,1]:
                mesh('/Game/Assets/Rock_01','TrailRock_{}_{}_{}_{}'.format(k,j,i,side),x-side*dy/length*270,y+side*dx/length*270,height=random.uniform(25,45),yaw=random.uniform(0,360),collision=False)
for i,(x,y,h) in enumerate([(114000,28400,650),(114500,28900,850),(115000,28800,600),(114300,29500,450)]):
    mesh('/Game/Assets/Rock_0'+str(i%4+1),'EncounterOutcrop_'+str(i),x,y,height=h,yaw=random.uniform(0,360))

# Physical enclosure is beyond the current normal-patrol boundary.
# Rock and shrub clusters make the restriction visible; hidden collision closes gaps.
x0,x1,y0,y1=101500,121000,16000,35000
for name,x,y,sx,sy in [('West',(x0),25500,1,190),('East',x1,25500,1,190),('South',111250,y0,195,1),('North',111250,y1,195,1)]:
    wall=spawn(unreal.StaticMeshActor,'Boundary_'+name,x,y,0,0,'Boundaries')
    wall.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
    wall.set_actor_location(unreal.Vector(x,y,400),False,False)
    wall.set_actor_scale3d(unreal.Vector(sx,sy,50))
    wall.static_mesh_component.set_collision_profile_name('BlockAll')
    wall.set_actor_hidden_in_game(True)
for edge in range(4):
    for i in range(18):
        t=(i+.5)/18
        x,y=(x0+300,y0+(y1-y0)*t) if edge==0 else (x1-300,y0+(y1-y0)*t) if edge==1 else (x0+(x1-x0)*t,y0+300) if edge==2 else (x0+(x1-x0)*t,y1-300)
        x+=random.uniform(-130,130); y+=random.uniform(-130,130)
        mesh('/Game/Assets/Rock_0'+str(i%4+1),'BoundaryRock_{}_{}'.format(edge,i),x,y,height=random.uniform(450,850),yaw=random.uniform(0,360))
        mesh('/Game/Assets/Bush','BoundaryBush_{}_{}'.format(edge,i),x+random.uniform(-250,250),y+random.uniform(-250,250),height=random.uniform(160,260),yaw=random.uniform(0,360),collision=False)
        if i%3==0: mesh('/Game/Foliage/Acacia/Acacia1','BoundaryTree_{}_{}'.format(edge,i),x+450,y,height=random.uniform(800,1150),yaw=random.uniform(0,360))
# Sparse trees keep the encounter and both approaches readable.
for i,(x,y) in enumerate([(107500,18300),(109500,20000),(108000,26000),(105000,27500),(106500,31500),(110000,32000),(119500,28000),(119700,33000),(112500,18000),(117000,19000),(117500,23500),(109000,28500)]):
    path='/Game/Assets/MopaneTree' if i%2 else '/Game/Foliage/Acacia/Acacia1'
    mesh(path,'ShadeTree_'+str(i),x,y,height=random.uniform(750,1100),yaw=random.uniform(0,360))
# Reuse the existing fence mesh as the closed future exit at camp.
for i in range(4): mesh('/Game/Assets/Human_Stuff/Temp_Fence','CampFence_'+str(i),102100,18100+i*350,height=220,yaw=90)
sign('RESERVE ACCESS - CLOSED','GateSign',102500,19300,0)

# A separate navigation bounds volume covers the tutorial. Existing bounds remain unchanged.
nav_source=next(a for a in original if a.get_class().get_name()=='NavMeshBoundsVolume')
nav=editor.duplicate_actor(nav_source,world,unreal.Vector(0,0,0));label(nav,'Navigation','Navigation')
_,extent=nav.get_actor_bounds(False);scale=nav.get_actor_scale3d()
nav.set_actor_scale3d(unreal.Vector(scale.x*10300/extent.x,scale.y*10000/extent.y,scale.z*3000/extent.z))
nav.set_actor_location(unreal.Vector(111250,25500,0),False,False)
# Save additions and owning map; no landscape/foliage source assets are edited.
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
manifest=[dict(label=a.get_actor_label(),path=a.get_path_name(),position=str(a.get_actor_location())) for a in created]
with open(unreal.Paths.project_saved_dir()+'Codex/tutorial_placement.json','w') as f:json.dump(manifest,f,indent=2)
unreal.log('TUTORIAL PLACED {} actors'.format(len(created)))
unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(100000,14000,4500),unreal.Rotator(pitch=-23,yaw=45,roll=0))


unreal.SystemLibrary.execute_console_command(world, 'Automation RunTests KrugerConclusion.PoachingPatrol')
