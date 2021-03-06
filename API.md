## Lua Public API

### Scripts
Scripts modify an entity's operation.

There are four types of script execution. Ordered from most common to least, here they are:
- Triggered: Only runs when the script is called by _another_ script. Blocks the execution
of the caller script until the trigger is finished. This means that if the caller script is
a parallel one, the main auto/LP auto coroutine will not be affected.

- Auto: Automatically runs once, when the entity is created (Or loaded, if it is part of the
map.) Blocks the execution of other auto/LP auto scripts. If many auto scripts are created on
the same frame (For example, on map load), there is no defined order of execution between
them. As such, refrain from using more than one auto script per map.

- LP Auto: Stands for "Low Priority" auto. Works exactly as an auto script, but if a Low
Priority auto script and an auto script are loaded on the same frame, the auto script will
get priority. Normally used for player scripts and nothing more.

- Parallel Auto: Automatically runs once, when the entity is created (Or loaded, if it is
part of the map.) and **doesn't block any other scripts when doing so**, since it creates
its own coroutine; effectively running parallel to other scripts. **Parallel scripts should
be only used if you truly know what you're doing, as they can cause strange side effects and
synchronization issues.**

- Parallel Triggered: Only runs when the script is called by _another_ script. Does **not**
block the execution of the caller script, since it creates its own coroutine; effectively
running parallel to other scripts. **Parallel scripts should be only used if you truly know
what you're doing, as they can cause strange side effects and synchronization issues.**

Here's a sample script that makes the camera follow the parent entity indefinitely:
```lua
while true do
    camera.pos = entity.pos
    -- Yielding is needed here to return control to the game and
    -- allow to execute other scripts/functions.
    coroutine.yield()
end
```

### Data Structures
All data structures are contained in the `game` table.
#### Vec2
Stores a 2D point in space.

Pseudodefinition:
```
data Vec2 {
    /// Creates a new Vec2 from a X and Y component.
    new(float x, float y);

    float x,y { get; set; }
}
```
#### IVec2
Stores an 2D point in space composed by integers.

Pseudodefinition:
```
data IVec2 {
    /// Creates a new IVec2 from a X and Y component.
    new(int x, int y);

    int x,y { get; set; }
}
```
#### CameraClass
Defines the position, scale and effects of the camera.

There must always be a single instance of it, named `game.camera`.

Pseudodefinition:
```
data CameraClass {
    /// Position (Measured in tiles)
    Vec2 pos { get; set; }
    /// Camera zoom, measured in screen pixels per asset pixels. (i.e. 2 => 200% zoom.)
    float zoom { get; set; }
} camera;
```
#### Sprite
Defines a textured and named asset that contains a pivot. Used in entities.

Pseudodefinition:
```
data Sprite {
    /// Pivot of the sprite, aka where it scales/rotates/translates from.
    /// {0,0} means upper left corner of the image, {1,1} means lower right.
    Vec2 pivot { get; set; }

    string name { get; }
    IVec2 size_in_pixels { get; }
};
```
#### Entity
Defines a dynamic object that may move and change state and has a sprite and scripts attached to it.

If the script is a child of an entity, it may access it via the readonly `entity` property.

Pseudodefinition:
```
data Entity {
    /// Position (Measured in tiles)
    Vec2 pos { get; set; }
    Sprite sprite { get; set; }
    string name { get; set; }
};
```
#### ScreenLayer
Defines an object that has a drawing callback and an order. Examples of these objects can be, for example, the map view,
an UI menu, etc.

Pseudodefinition:
```
data ScreenLayer {
    /// Creates a new ScreenLayer that is automatically added to the ScreenLayer list.
    /// ScreenLayers start visible and on the front.
    new(function render_callback);
    bool visible { get; set; }
    function render_callback { get; set; }
    
    /// Sends the screen layer to the top of the render queue, and thus is rendered last, after other layers.
    void to_front();
    /// Sends the screen layer to the top of the render queue, and thus is rendered last, in front of other layers.
    void to_back();
};
```

### The game table
The game table contains everything the API defines, including all data structures. Apart from them, it
also defines the following data:
```
table game {
    /// Returns all the screenlayers created via new() or internally, which includes both hidden and visible ones.
    ScreenLayer[] get_all_screen_layers();
    ScreenLayer[] get_visible_screen_layers();
    ScreenLayer[] get_hidden_screen_layers();

    /// A global instance of the Camera class.
    Camera camera;

    /// Explained later.
    table input { ... }

    /// Explained later.
    table assets { ... }
}
```

#### Assets
You can use the `game.assets` function table to load resources that the game contains,
such as sprites, textures, etc.

Pseudodefinition:
```
table assets {
    /// Takes a resource path relative to the main game data / project path and returns the asset
    /// related to it, or nil if the path is not recognized or not associated with a type.
    any load(string path);
};
```
#### Input
The `game.input` table contains many functions and utils to get user input.

Pseudodefinition:
```
table input {
    /// An enumeration table containing all possible keys being held.
    table keys {
        A = 0,
        B = 1,
        C = 2,
        // ...
    };
    
    /// Contains the state of a key.
    data KeyState {
        /// True if the key is pressed, false otherwise.
        bool held { get; }
        /// True if the key press had begun on the last frame.
        bool just_pressed { get; }
        /// True if the key release had begun on the last frame.
        bool just_released { get; }
    };

    /// Returns the key state of a particular key from the keys table.
    KeyState get_key_state(key);
};
```