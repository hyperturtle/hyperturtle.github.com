(function() {
  var cc, checktile, clouds, cursor, cursors, draw, getnewcursor, getsim, getstats, getsurrounding, map, mapgen, mapsize, tile_temp, tt, tts, updatetile;

  tile_temp = _.template('\
<div\
  class="tile tile_<%=data.x%>_<%=data.y%> tiletype_<%=data.type%>"\
  data-robot=\'<%=JSON.stringify(data)%>\'>\
</div>');

  mapsize = 10;

  cursor = {
    drill: 0,
    laser: 1,
    grass: 2,
    plant: 3,
    tree: 4,
    forest: 5,
    cloud: 6
  };

  tt = {
    none: 0,
    ice: 1,
    water: 2,
    trash: 3,
    grass: 100,
    plant: 101,
    tree: 102,
    forest: 103
  };

  tts = _(tt).chain().values().map(function(x) {
    return 'tiletype_' + x;
  }).value().join(' ');

  cursors = _(cursor).chain().values().map(function(x) {
    return 'cursor_' + x;
  }).value().join(' ');

  cc = 0;

  map = [];

  clouds = [];

  mapgen = function() {
    return _(_.range(mapsize)).map(function(x) {
      return _(_.range(mapsize)).map(function(y) {
        return {
          type: Math.floor(Math.random() * 4),
          x: x,
          y: y
        };
      });
    });
  };

  draw = function() {
    var x, y, _ref, _results;
    _results = [];
    for (x = 0, _ref = mapsize - 1; 0 <= _ref ? x <= _ref : x >= _ref; 0 <= _ref ? x++ : x--) {
      _results.push((function() {
        var _ref2, _results2;
        _results2 = [];
        for (y = 0, _ref2 = mapsize - 1; 0 <= _ref2 ? y <= _ref2 : y >= _ref2; 0 <= _ref2 ? y++ : y--) {
          _results2.push(map[0][x][y].tile = $(tile_temp({
            data: {
              x: x,
              y: y,
              type: map[0][x][y].type
            }
          })).appendTo("#map"));
        }
        return _results2;
      })());
    }
    return _results;
  };

  getnewcursor = function() {
    cc = Math.floor(Math.random() * 3);
    return $("#cursor").removeClass(cursors).addClass('cursor_' + cc);
  };

  getsurrounding = function(x, y, type) {
    return _([[x + 1, y], [x - 1, y], [x, y + 1], [x, y - 1]]).chain().filter(function(x) {
      return x[0] >= 0 && x[0] < mapsize;
    }).filter(function(x) {
      return x[1] >= 0 && x[1] < mapsize;
    }).filter(function(x) {
      return type(map[0][x[0]][x[1]].type);
    }).value();
  };

  getsim = function(x, y, type) {
    var old_length, point, points, _i, _len;
    points = _.union([[x, y]], getsurrounding(x, y, type));
    old_length = 0;
    while (old_length !== points.length) {
      old_length = points.length;
      for (_i = 0, _len = points.length; _i < _len; _i++) {
        point = points[_i];
        points = _.union(points, getsurrounding(point[0], point[1], type));
        points = _.unique(points, false, function(x) {
          return x[0] + ',' + x[1];
        });
      }
    }
    return points;
  };

  updatetile = function(tile) {
    return tile.tile.removeClass(tts).addClass('tiletype_' + tile.type);
  };

  checktile = function(tile) {
    var blob, ti, type, _i, _len;
    switch (tile.type) {
      case tt.grass:
      case tt.plant:
      case tt.tree:
      case tt.forest:
        blob = getsim(tile.x, tile.y, function(x) {
          return x === tile.type || x === tt.water;
        });
        break;
      default:
        return;
        blob = getsim(tile.x, tile.y, function(x) {
          return x === tile.type;
        });
    }
    switch (tile.type) {
      case tt.grass:
      case tt.plant:
      case tt.tree:
      case tt.forest:
      case tt.water:
        if (blob.length >= 3) {
          type = tile.type;
          for (_i = 0, _len = blob.length; _i < _len; _i++) {
            ti = blob[_i];
            map[0][ti[0]][ti[1]].type = tt.none;
            updatetile(map[0][ti[0]][ti[1]]);
          }
          type = Math.min(tt.tree, type + Math.floor(blob.length / 3) - 1);
          console.log(type);
          switch (type) {
            case tt.tree:
              tile.type = tt.forest;
              break;
            case tt.plant:
              tile.type = tt.tree;
              break;
            case tt.grass:
              tile.type = tt.plant;
          }
          updatetile(tile);
          return true;
        }
    }
    return false;
  };

  getstats = function() {
    var stats, x, y, _ref, _ref2;
    stats = {};
    for (x = 0, _ref = mapsize - 1; 0 <= _ref ? x <= _ref : x >= _ref; 0 <= _ref ? x++ : x--) {
      for (y = 0, _ref2 = mapsize - 1; 0 <= _ref2 ? y <= _ref2 : y >= _ref2; 0 <= _ref2 ? y++ : y--) {
        stats[map[0][x][y].type] = stats[map[0][x][y].type] || 0;
        stats[map[0][x][y].type] += 1;
      }
    }
    return stats;
  };

  $(function() {
    map.push(mapgen());
    getnewcursor();
    draw();
    return $('.tile').live('click', function() {
      var consumed, data, tile;
      data = $(this).data('robot');
      tile = map[0][data.x][data.y];
      consumed = true;
      switch (cc) {
        case cursor.grass:
          switch (tile.type) {
            case tt.none:
              tile.type = tt.grass;
              break;
            default:
              consumed = false;
          }
          break;
        case cursor.drill:
          switch (tile.type) {
            case tt.trash:
              tile.type = tt.none;
              break;
            case tt.grass:
            case tt.plant:
            case tt.tree:
            case tt.none:
              tile.type = tt.none;
              break;
            default:
              consumed = false;
          }
          break;
        case cursor.laser:
          switch (tile.type) {
            case tt.ice:
              tile.type = tt.water;
              break;
            case tt.water:
            case tt.none:
              tile.type = tt.none;
          }
          break;
        default:
          consumed = false;
      }
      if (consumed) {
        updatetile(tile);
        while (true) {
          if (!checktile(tile)) break;
        }
        getnewcursor();
      }
      return console.log(getstats());
    });
  });

}).call(this);
