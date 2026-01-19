# 🔍 Diagnostic des Problèmes Visuels

## Symptômes Rapportés
- ❌ Pas de barres de vie visibles
- ❌ Pas de boss qui apparaît
- ❌ Pas d'animations visibles

## Causes Probables et Solutions

### 1. ❌ Les Assets N'existent Pas

**Symptôme**: Le jeu fonctionne mais rien ne s'affiche/anime

**Vérification**:
```bash
# Vérifier si les dossiers d'animation existent
ls -la assets/sprites/basic_ennemie/
ls -la assets/sprites/boss_sprite/
ls -la assets/sprites/boss_shoot/
ls -la assets/font/star-crush/
```

**Solution**: Si les dossiers sont vides ou n'existent pas, les animations ne peuvent pas fonctionner. Les health bars devraient quand même s'afficher mais le boss/ennemis n'auront pas de sprite.

---

### 2. ❌ Le Boss Ne Spawn Jamais

**Cause**: Personne n'atteint 500 points

**Vérification dans le code**:
- `server/src/GameModule.cpp` ligne 88: `if (s.getPoints() >= BOSS_SPAWN_SCORE)`
- `BOSS_SPAWN_SCORE = 500` (défini dans GameModule.hpp)
- Chaque ennemi tué donne 100 points

**Math**: Il faut tuer **5 ennemis minimum** pour spawn le boss

**Solution Temporaire - Réduire le Score Requis**:
```cpp
// Dans server/include/GameModule.hpp ligne 119
static constexpr uint32_t BOSS_SPAWN_SCORE = 100; // Au lieu de 500
```

---

### 3. ❌ Les Health Bars Ne S'affichent Pas

**Problèmes Possibles**:

#### A. Les entités n'ont pas de Sprite

Dans `game/src/GameLoop.cpp`, les lignes 237 et 262 créent des Sprites mais la variable `sprite` n'est pas utilisée après. **C'est normal** - le Sprite est ajouté au registry et sera utilisé par le RenderSystem.

#### B. Bug dans PlayerHealthBarSystem

Regardons le code:
```cpp
// engine/src/systems/PlayerHealthBarSystem.cpp ligne 20-24
registry.each<Transform, Sprite, Health>(
    [&](EntityID id, Transform& t, Sprite& s, Health& h) {
        if (registry.has<Enemy>(id)) {  // ← Skip si c'est un ennemi
            return;
        }
```

**PROBLÈME IDENTIFIÉ**: Les joueurs n'ont **PAS** de component `Sprite` dans le registry côté client dans un jeu réseau !

Dans GameLoop.cpp ligne 222, on ajoute un Sprite aux joueurs réseau:
```cpp
Sprite& sprite = registry.add<Sprite>(local_entity, texturePath);
```

Donc les Sprites existent. Mais est-ce que `PlayerHealthBarSystem` utilise le bon ordre de components ?

---

### 4. ⚠️ Ordre d'Affichage

Les health bars doivent être rendues **APRÈS** les sprites pour être au-dessus.

Dans `game/src/GameClientHelpers.cpp` lignes 154-160:
```cpp
systems.push_back(std::make_unique<MovementSystem>());
systems.push_back(std::make_unique<ScrollingBackgroundSystem>(width));
systems.push_back(std::make_unique<RenderSystem>());  // Render sprites
systems.push_back(std::make_unique<PlayerHealthBarSystem>());  // Render health bars AFTER
systems.push_back(std::make_unique<EnemyHealthBarSystem>());
systems.push_back(std::make_unique<BossHealthBarSystem>());
systems.push_back(std::make_unique<ScoreDisplaySystem>(scoreProvider));
```

✅ **L'ordre est correct** - les health bars sont après le RenderSystem.

---

## 🔧 Corrections à Apporter

### Correction 1: Ajouter des Logs de Debug

Ajoutons des logs pour voir ce qui se passe:

**Dans `server/src/GameModule.cpp`** ligne 90:
```cpp
if (s.getPoints() >= BOSS_SPAWN_SCORE) {
    std::cout << "[GameModule] !!! BOSS SPAWN !!! Player score: " << s.getPoints() << std::endl;
    _bossSpawned = true;
    spawnBoss();
    break;
}
```

**Dans `game/src/GameLoop.cpp`** ligne 257 (quand boss reçu):
```cpp
} else if (update.entity_type == 5) {
    std::cout << "[GameLoop] BOSS ENTITY RECEIVED! HP=" << update.hp_max << std::endl;
    const auto& frames = getBossFrames();
```

**Dans `engine/src/systems/BossHealthBarSystem.cpp`** ligne 30:
```cpp
registry.each<Enemy, Health>([&](EntityID, Enemy& enemy, Health& h) {
    if (enemy.type == EnemyType::Boss) {
        std::cout << "[BossHealthBar] Rendering boss HP: " << h.current << "/" << h.max << std::endl;
        bossHealth = &h;
    }
});
```

---

### Correction 2: Réduire le Score Boss pour Tests

**Fichier**: `server/include/GameModule.hpp` ligne 119

**Avant**:
```cpp
static constexpr uint32_t BOSS_SPAWN_SCORE = 500;
```

**Après (pour tests)**:
```cpp
static constexpr uint32_t BOSS_SPAWN_SCORE = 100;  // Seulement 1 ennemi à tuer
```

---

### Correction 3: Vérifier que les Entités Réseau Ont les Bons Components

Le problème pourrait être que les entités réseau ne sont pas créées avec tous les components nécessaires.

**Vérification** dans `game/src/GameLoop.cpp`:

Pour les **joueurs** (ligne 220-231):
- ✅ Transform (ligne 219)
- ✅ Sprite (ligne 222)
- ✅ Health (ligne 227-231)

Pour les **ennemis** (ligne 232-256):
- ✅ Transform (ligne 219)
- ✅ Sprite (ligne 237)
- ✅ Animation (ligne 239)
- ✅ Enemy (ligne 250)
- ✅ Health (ligne 252-256)

Pour le **boss** (ligne 257-281):
- ✅ Transform (ligne 219)
- ✅ Sprite (ligne 262)
- ✅ Animation (ligne 267)
- ✅ Enemy (ligne 275)
- ✅ Health (ligne 277-281)

**Tout semble correct !**

---

## 🧪 Plan de Test

1. **Ajouter les logs** (Corrections 1)
2. **Réduire le score boss** à 100 (Correction 2)
3. **Recompiler**:
   ```bash
   cd build
   cmake --build . -j4
   ```
4. **Lancer le serveur et le client**
5. **Tuer 1 ennemi** (devrait donner 100 points)
6. **Vérifier les logs**:
   - "[GameModule] !!! BOSS SPAWN !!!"
   - "[GameLoop] BOSS ENTITY RECEIVED!"
   - "[BossHealthBar] Rendering boss HP: ..."

---

## 🎯 Checklist de Vérification

Quand vous lancez le jeu, vérifiez:

- [ ] Les joueurs ont une barre de vie cyan au-dessus d'eux
- [ ] Les ennemis ont une barre de vie au-dessus d'eux
- [ ] Les ennemis sont animés (frames changent)
- [ ] Après 100 points (1 ennemi tué), le boss spawn
- [ ] Le boss apparaît et est animé
- [ ] Une grosse barre rouge avec "BOSS1" apparaît en bas
- [ ] Les logs de debug s'affichent dans la console

---

## 🐛 Si Ça Ne Marche Toujours Pas

Si après avoir appliqué ces corrections rien ne change:

1. **Partagez les logs** de la console (serveur et client)
2. **Vérifiez les assets**: `ls assets/sprites/boss_sprite/` doit montrer des fichiers .png
3. **Testez en mode debug** avec des breakpoints
4. **Vérifiez les FPS** - si le jeu lag énormément, les animations peuvent ne pas être visibles

---

**Prochaine Étape**: Appliquer les corrections ci-dessus et tester !
