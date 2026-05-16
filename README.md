# Airwindows for Schwung

Bundles 500+ Airwindows audio effects (Chris Johnson's classic free plugin
suite) as a single audio FX module for the Ableton Move via Schwung / Move
Everything.

Built on a generic CLAP host: you can also drop other ARM64 Linux `.clap`
files into the plugins directory, though Airwindows is what ships by default.

## Prerequisites

- [Schwung / Move Everything](https://github.com/charlesvestal/move-everything) installed on your Ableton Move
- SSH access enabled: http://move.local/development/ssh

## Features

- 500+ Airwindows effects (via baconpaul/airwin2rack), grouped by category
- Category-based browsing (Consoles, Filter, Tape, Reverb, etc.)
- Per-plugin parameter control via encoder banks
- Usable as an audio FX in Signal Chain
- Extensible: drop additional ARM64 Linux `.clap` files into the plugins directory

## Important: Plugin Compatibility

**The Move has specific requirements for CLAP plugins:**

- ARM64 Linux (aarch64) binaries only
- No GUI dependencies (X11, Cairo, OpenGL, GTK, etc.)
- glibc 2.35 or older (Move's system library version)
- GLIBCXX 3.4.29 or older

Most pre-built CLAP plugins are x86_64 and include GUI code, so you'll typically need to **build from source** with headless options.

### When to Use CLAP vs Native Ports

| Use CLAP Host | Consider Native Port |
|---------------|---------------------|
| Plugin builds easily headless | Complex dependency chains |
| Minimal dependencies | JUCE-based with glibc issues |
| Effects and simple synths | Major synths you'll use often |
| Quick experimentation | Need deep Move UI integration |

For complex synths like Surge XT or Vital, the effort to resolve glibc/libstdc++ compatibility issues is often comparable to porting the DSP core natively.

## Installation

### From Release

```bash
./scripts/install.sh
```

### From Source

```bash
./scripts/build.sh
./scripts/install.sh
```

## Usage

1. Copy `.clap` plugin files to `/data/UserData/move-anything/modules/sound_generators/clap/plugins/` on the Move
2. If plugins need shared libraries, copy `.so` files to the same directory
3. Select the CLAP module from the host menu
4. Use the UI to browse and load plugins
5. Adjust parameters with the encoders

## Building Plugins

See [BUILDING.md](BUILDING.md) for detailed build instructions for specific plugin frameworks (SA_Toolkit, LSP Plugins, clap-plugins, etc.).

## Bundled Plugins

### Airwindows

This module bundles [Airwindows](https://github.com/airwindows/airwindows) by Chris Johnson — 509 high-quality open source audio effects as a single CLAP plugin. The CLAP build is from [airwin2rack](https://github.com/baconpaul/airwin2rack) by Bacon Pauls.

Airwindows is licensed under the MIT License. See [LICENSES/AIRWINDOWS.txt](LICENSES/AIRWINDOWS.txt) for full license text.

#### Plugin Reference

All 509 bundled Airwindows effects, in the order they appear in the browser. Numbers are the 1-based preset position.

<details>
<summary><strong>Ambience</strong> (25) — presets 1–25</summary>

1. ADT
2. BrightAmbience
3. BrightAmbience2
4. BrightAmbience3
5. Chorus
6. ChorusEnsemble
7. ClearCoat
8. Doublelay
9. Ensemble
10. Hombre
11. kChamberAR
12. Melt
13. MV
14. MV2
15. PitchDelay
16. PurestEcho
17. SampleDelay
18. StarChild
19. StarChild2
20. StereoChorus
21. StereoDoubler
22. StereoEnsemble
23. TapeDelay
24. TapeDelay2
25. TripleSpread

</details>

<details>
<summary><strong>Amp Sims</strong> (18) — presets 26–43</summary>

26. BassAmp
27. BassDrive
28. BigAmp
29. Cabs
30. Cabs2
31. ChimeyDeluxe
32. ChimeyGuitar
33. ChimeyGuitar2
34. CrickBass
35. FireAmp
36. GrindAmp
37. LeadAmp
38. LilAmp
39. MidAmp
40. PointyDeluxe
41. PointyGuitar
42. PunchyGuitar
43. Wolfbot

</details>

<details>
<summary><strong>Bass</strong> (10) — presets 44–53</summary>

44. BassKit
45. DubCenter
46. DubSub
47. DubSub2
48. Elliptical
49. FathomFive
50. Floor
51. Hermepass
52. Infrasonic
53. OrbitKick

</details>

<details>
<summary><strong>Biquads</strong> (9) — presets 54–62</summary>

54. Biquad
55. Biquad2
56. BiquadDouble
57. BiquadHiLo
58. BiquadNonLin
59. BiquadOneHalf
60. BiquadPlus
61. BiquadStack
62. BiquadTriple

</details>

<details>
<summary><strong>Brightness</strong> (26) — presets 63–88</summary>

63. Acceleration
64. Acceleration2
65. Air
66. Air2
67. Air3
68. Air4
69. DeBess
70. DeEss
71. DeHiss
72. EverySlew
73. GoldenSlew
74. Hypersonic
75. HypersonX
76. PlatinumSlew
77. PurestAir
78. Sinew
79. Slew
80. Slew2
81. Slew3
82. Slew4
83. SlewSonic
84. Smooth
85. Ultrasonic
86. UltrasonicLite
87. UltrasonicMed
88. UltrasonX

</details>

<details>
<summary><strong>Clipping</strong> (9) — presets 89–97</summary>

89. ADClip7
90. ADClip8
91. ADClip9
92. AQuickVoiceClip
93. ClipOnly2
94. ClipOnly3
95. ClipSoftly
96. FinalClip
97. OneCornerClip

</details>

<details>
<summary><strong>Consoles</strong> (56) — presets 98–153</summary>

98. AtmosphereBuss
99. AtmosphereChannel
100. C5RawBuss
101. C5RawChannel
102. Console0Buss
103. Console0Channel
104. Console4Buss
105. Console4Channel
106. Console5Buss
107. Console5Channel
108. Console5DarkCh
109. Console6Buss
110. Console6Channel
111. Console7Buss
112. Console7Cascade
113. Console7Channel
114. Console7Crunch
115. Console8BussHype
116. Console8BussIn
117. Console8BussOut
118. Console8ChannelHype
119. Console8ChannelIn
120. Console8ChannelOut
121. Console8LiteBuss
122. Console8LiteChannel
123. Console8SubHype
124. Console8SubIn
125. Console8SubOut
126. Console9Buss
127. Console9Channel
128. ConsoleHBuss
129. ConsoleHChannel
130. ConsoleHPre
131. ConsoleLABuss
132. ConsoleLAChannel
133. ConsoleMCBuss
134. ConsoleMCChannel
135. ConsoleMDBuss
136. ConsoleMDChannel
137. ConsoleX2Buss
138. ConsoleX2Channel
139. ConsoleX2Pre
140. ConsoleXBuss
141. ConsoleXChannel
142. ConsoleXPre
143. CStrip
144. EveryConsole
145. PDBuss
146. PDChannel
147. PurestConsole2Buss
148. PurestConsole2Channel
149. PurestConsole3Buss
150. PurestConsole3Channel
151. PurestConsoleBuss
152. PurestConsoleChannel
153. X2Buss

</details>

<details>
<summary><strong>Distortion</strong> (15) — presets 154–168</summary>

154. Density
155. Density2
156. Density3
157. Dirt
158. Distortion
159. Drive
160. Edge
161. HardVacuum
162. HighImpact
163. Loud
164. Mackity
165. MultiBandDistortion
166. PunchyDeluxe
167. Zoom
168. ZOutputStage

</details>

<details>
<summary><strong>Dithers</strong> (23) — presets 169–191</summary>

169. Beam
170. BuildATPDF
171. Dark
172. Ditherbox
173. DitherFloat
174. DitherMeDiskers
175. DitherMeTimbers
176. DoublePaul
177. HighGlossDither
178. NaturalizeDither
179. NodeDither
180. NotJustAnotherCD
181. NotJustAnotherDither
182. PaulDither
183. PaulWide
184. RawGlitters
185. RawTimbers
186. SpatializeDither
187. StudioTan
188. TapeDither
189. TPDFDither
190. TPDFWide
191. VinylDither

</details>

<details>
<summary><strong>Dynamics</strong> (34) — presets 192–225</summary>

192. BeziComp
193. BlockParty
194. BrassRider
195. ButterComp
196. ButterComp2
197. Compresaturator
198. curve
199. DigitalBlack
200. DrumSlam
201. Dynamics
202. Dynamics2
203. Dynamics3
204. Gatelope
205. Logical4
206. Overheads
207. Podcast
208. PodcastDeluxe
209. Point
210. Pop
211. Pop2
212. Pop3
213. Pressure4
214. Pressure5
215. Pressure6
216. PurestSquish
217. Pyewacket
218. Recurve
219. SoftGate
220. StoneFireComp
221. Surge
222. SurgeTide
223. Swell
224. Thunder
225. VariMu

</details>

<details>
<summary><strong>Effects</strong> (29) — presets 226–254</summary>

226. Aura
227. CloudCoat
228. Disintegrate
229. Dubly
230. Dubly2
231. Dubly3
232. Energy
233. Energy2
234. Exciter
235. Facet
236. Fracture
237. Fracture2
238. GalacticVibe
239. GlitchShifter
240. Gringer
241. GuitarConditioner
242. Nikola
243. Pafnuty
244. Pafnuty2
245. PitchNasty
246. PowerSag
247. PowerSag2
248. Preponderant
249. RingModulator
250. ShortBuss
251. TakeCare
252. Tremolo
253. TremoSquare
254. Trianglizer

</details>

<details>
<summary><strong>Filter</strong> (51) — presets 255–305</summary>

255. AngleEQ
256. AngleFilter
257. Average
258. AverMatrix
259. Baxandall
260. Baxandall2
261. Baxandall3
262. BezEQ
263. BezEQ2
264. BezEQ3
265. Capacitor
266. Capacitor2
267. CStrip2
268. Dattorro
269. Distance
270. Distance2
271. Distance3
272. Donut
273. EQ
274. FatEQ
275. Highpass
276. Highpass2
277. Holt
278. Holt2
279. Hull
280. Hull2
281. Isolator
282. Isolator2
283. Isolator3
284. Kalman
285. Lowpass
286. Lowpass2
287. MackEQ
288. Parametric
289. Pear
290. Pear2
291. PearEQ
292. PearLiteEQ
293. PrimeFIR
294. ResEQ
295. ResEQ2
296. Silken
297. SmoothEQ
298. SmoothEQ2
299. SmoothEQ3
300. Stonefire
301. SubTight
302. Suzan
303. TapeFat
304. ToneSlant
305. Weight

</details>

<details>
<summary><strong>Lo-Fi</strong> (19) — presets 306–324</summary>

306. Bite
307. BitGlitter
308. ChromeOxide
309. Cojones
310. CrunchyGrooveWear
311. DeBez
312. Deckwrecka
313. DeRez
314. DeRez2
315. DeRez3
316. DustBunny
317. Flutter
318. Flutter2
319. GrooveWear
320. HipCrush
321. Pockey
322. Pockey2
323. TapeBias
324. Vibrato

</details>

<details>
<summary><strong>Noise</strong> (9) — presets 325–333</summary>

325. DarkNoise
326. DeNoise
327. ElectroHat
328. Noise
329. Silhouette
330. TapeDust
331. Texturize
332. TexturizeMS
333. VoiceOfTheStarship

</details>

<details>
<summary><strong>Reverb</strong> (37) — presets 334–370</summary>

334. Chamber
335. Chamber2
336. CreamCoat
337. CrunchCoat
338. Galactic
339. Galactic2
340. Galactic3
341. Infinity
342. Infinity2
343. kAlienSpaceship
344. kBeyond
345. kCathedral
346. kCathedral2
347. kCathedral3
348. kCathedral4
349. kCathedral5
350. kCosmos
351. kGuitarHall
352. kGuitarHall2
353. kPlate140
354. kPlate240
355. kPlateA
356. kPlateB
357. kPlateC
358. kPlateD
359. kRockstar
360. kStation
361. kWoodRoom
362. MatrixVerb
363. NonlinearSpace
364. PocketVerbs
365. Reverb
366. Verbity
367. Verbity2
368. VerbSixes
369. VerbThic
370. VerbTiny

</details>

<details>
<summary><strong>Saturation</strong> (17) — presets 371–387</summary>

371. Creature
372. Desk4
373. Dyno
374. Focus
375. Huge
376. Hypersoft
377. Mojo
378. NCSeventeen
379. PurestDrive
380. PurestSaturation
381. Righteous4
382. Spiral
383. Spiral2
384. SquareRoot
385. Tube
386. Tube2
387. UnBox

</details>

<details>
<summary><strong>Stereo</strong> (13) — presets 388–400</summary>

388. AutoPan
389. BitDualPan
390. LRFlipTimer
391. MSFlipTimer
392. SideDull
393. Sidepass
394. Srsly
395. Srsly2
396. Srsly3
397. StereoFX
398. SweetWide
399. ToVinyl4
400. Wider

</details>

<details>
<summary><strong>Subtlety</strong> (19) — presets 401–419</summary>

401. Coils
402. Coils2
403. Desk
404. Discontapeity
405. Discontinuity
406. Hype
407. Inflamer
408. Interstage
409. Mastering
410. Mastering2
411. PhaseNudge
412. PurestWarm
413. PurestWarm2
414. Remap
415. Shape
416. SingleEndedTriode
417. Sweeten
418. TransDesk
419. TubeDesk

</details>

<details>
<summary><strong>Tape</strong> (12) — presets 420–431</summary>

420. FromTape
421. IronOxide5
422. IronOxideClassic
423. IronOxideClassic2
424. Tape
425. TapeHack
426. TapeHack2
427. ToTape5
428. ToTape6
429. ToTape7
430. ToTape8
431. ToTape9

</details>

<details>
<summary><strong>Tone Color</strong> (16) — presets 432–447</summary>

432. Apicolypse
433. BussColors4
434. Calibre
435. Channel4
436. Channel5
437. Channel6
438. Channel7
439. Channel8
440. Channel9
441. Cider
442. Crystal
443. Elation
444. Luxor
445. Neverland
446. Precious
447. WoodenBox

</details>

<details>
<summary><strong>Unclassified</strong> (1) — preset 448</summary>

448. Longhand

</details>

<details>
<summary><strong>Utility</strong> (37) — presets 449–485</summary>

449. Balanced
450. BitShiftGain
451. BitShiftPan
452. Cans
453. CansAW
454. ContentHideD
455. DCVoltage
456. DeCrackle
457. DubPlate
458. DubPlate2
459. EdIsDim
460. EveryTrim
461. Flipity
462. Golem
463. HermeTrim
464. LeftoMono
465. LRConvolve
466. LRConvolve2
467. LRConvolve3
468. MidSide
469. Monitoring
470. Monitoring2
471. Monitoring3
472. MoNoam
473. PeaksOnly
474. PurestDualPan
475. PurestFade
476. PurestGain
477. RightoMono
478. SlewOnly
479. SoftClock
480. SoftClock2
481. SoftClock3
482. SubsOnly
483. uLawDecode
484. uLawEncode
485. VoiceTrick

</details>

<details>
<summary><strong>XYZ Filters</strong> (24) — presets 486–509</summary>

486. XBandpass
487. XHighpass
488. XLowpass
489. XNotch
490. XRegion
491. YBandpass
492. YHighpass
493. YLowpass
494. YNotBandpass
495. YNotch
496. YNotHighpass
497. YNotLowpass
498. YNotNotch
499. ZAcidLowpass
500. ZBandpass
501. ZBandpass2
502. ZHighpass
503. ZHighpass2
504. ZLowpass
505. ZLowpass2
506. ZNotch
507. ZNotch2
508. ZRegion
509. ZRegion2

</details>

## License

MIT License - see LICENSE file

## AI Assistance Disclaimer

This module is part of Move Everything and was developed with AI assistance, including Claude, Codex, and other AI assistants.

All architecture, implementation, and release decisions are reviewed by human maintainers.  
AI-assisted content may still contain errors, so please validate functionality, security, and license compatibility before production use.
