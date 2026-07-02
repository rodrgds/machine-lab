import React from 'react';
import {
  AbsoluteFill,
  Easing,
  Img,
  interpolate,
  staticFile,
  useCurrentFrame,
  useVideoConfig,
} from 'remotion';

type Scene = {
  start: number;
  duration: number;
  render: (localFrame: number, index: number) => React.ReactNode;
};

type Shot = {
  title: string;
  subtitle: string;
  image: string;
  accent: string;
};

const colors = {
  bg: '#0e141b',
  panel: '#17222b',
  panel2: '#212b36',
  ink: '#f6f7f2',
  muted: '#a9b5c3',
  teal: '#42d9c8',
  yellow: '#f4c95d',
  coral: '#f27c6a',
  violet: '#9d7cff',
  green: '#6bd67a',
  blue: '#7cc7ff',
};

const screenshots: Shot[] = [
  {
    title: 'Write a tiny editor',
    subtitle: 'keyboard state, cursor logic, framebuffer UI',
    image: 'screenshots/word-processor.png',
    accent: colors.blue,
  },
  {
    title: 'Build a music maker',
    subtitle: 'tracker grid, tempo, mixer, generated PCM',
    image: 'screenshots/music-maker.png',
    accent: colors.violet,
  },
  {
    title: 'Ship arcade loops',
    subtitle: 'timer physics, collision, HUD, replay scripts',
    image: 'screenshots/breakout.png',
    accent: colors.yellow,
  },
  {
    title: 'Record compact games',
    subtitle: 'input traces, audio hooks, captured frames',
    image: 'screenshots/flappy.png',
    accent: colors.green,
  },
  {
    title: 'Scale to real projects',
    subtitle: 'menus, profiling, audio, UART multiplayer',
    image: 'screenshots/ninjix.png',
    accent: colors.coral,
  },
];

const snippets = [
  {
    label: 'RTC',
    code: [
      'lcom_port_write8(0x70, RTC_REG_A);',
      'lcom_port_read8(0x71, &status);',
      'if ((status & RTC_UIP) == 0) {',
      '  rtc_read_date(&date);',
      '}',
    ],
    color: colors.teal,
  },
  {
    label: 'EVENT LOOP',
    code: [
      'while (running) {',
      '  wait_for_irq(&event);',
      '  update_state(event);',
      '  render_frame();',
      '}',
    ],
    color: colors.yellow,
  },
  {
    label: 'FRAMEBUFFER',
    code: [
      'pixel = fb + y * pitch + x * bytes;',
      'draw_rect(panel, color);',
      'draw_xpm(sprite, x, y);',
      'video_present();',
    ],
    color: colors.coral,
  },
];

const labs = [
  ['RTC', 'bit fields', colors.teal],
  ['PIT', 'timer IRQs', colors.yellow],
  ['KBD', 'scancodes', colors.blue],
  ['MOUSE', 'packets', colors.violet],
  ['VBE', 'pixels', colors.coral],
  ['PCM', 'audio', colors.green],
  ['UART', 'byte streams', '#ff9f7a'],
] as const;

const clamp = {
  extrapolateLeft: 'clamp' as const,
  extrapolateRight: 'clamp' as const,
};

const ease = Easing.bezier(0.16, 1, 0.3, 1);

const fade = (frame: number, start: number, end: number) =>
  interpolate(frame, [start, end], [0, 1], {...clamp, easing: ease});

const exitFade = (frame: number, start: number, end: number) =>
  interpolate(frame, [start, end], [1, 0], {...clamp, easing: ease});

const enterY = (frame: number, delay = 0, amount = 42) =>
  interpolate(frame, [delay, delay + 26], [amount, 0], {...clamp, easing: ease});

const sceneOpacity = (frame: number, duration: number) =>
  Math.min(fade(frame, 0, 18), exitFade(frame, duration - 20, duration));

const bg = (frame: number, accent = colors.teal): React.CSSProperties => {
  const x = interpolate(Math.sin(frame / 34), [-1, 1], [12, 88]);
  const y = interpolate(Math.cos(frame / 47), [-1, 1], [18, 82]);
  return {
    background:
      `radial-gradient(circle at ${x}% ${y}%, ${accent}26, transparent 32%), ` +
      `linear-gradient(135deg, ${colors.bg}, #121822 42%, #1b1f2b)`,
  };
};

const Shell: React.FC<{
  children: React.ReactNode;
  frame: number;
  duration: number;
  accent?: string;
}> = ({children, frame, duration, accent = colors.teal}) => (
  <AbsoluteFill
    style={{
      ...bg(frame, accent),
      color: colors.ink,
      fontFamily:
        'Inter, ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont, sans-serif',
      opacity: sceneOpacity(frame, duration),
      overflow: 'hidden',
    }}
  >
    <GridLines frame={frame} />
    <div
      style={{
        position: 'absolute',
        inset: 72,
        border: `1px solid ${accent}35`,
        borderRadius: 34,
        boxShadow: `0 0 80px ${accent}12 inset`,
      }}
    />
    {children}
  </AbsoluteFill>
);

const GridLines: React.FC<{frame: number}> = ({frame}) => {
  const shift = frame % 48;
  return (
    <AbsoluteFill
      style={{
        opacity: 0.16,
        backgroundImage:
          'linear-gradient(rgba(255,255,255,0.12) 1px, transparent 1px), linear-gradient(90deg, rgba(255,255,255,0.12) 1px, transparent 1px)',
        backgroundSize: '48px 48px',
        backgroundPosition: `${shift}px ${shift}px`,
        maskImage: 'radial-gradient(circle at center, black, transparent 78%)',
      }}
    />
  );
};

const Kicker: React.FC<{children: React.ReactNode; color?: string}> = ({
  children,
  color = colors.teal,
}) => (
  <div
    style={{
      color,
      fontFamily: '"SFMono-Regular", "Roboto Mono", Consolas, monospace',
      fontSize: 26,
      fontWeight: 700,
      letterSpacing: 2,
      textTransform: 'uppercase',
    }}
  >
    {children}
  </div>
);

const BigTitle: React.FC<{children: React.ReactNode; size?: number}> = ({
  children,
  size = 112,
}) => (
  <div
    style={{
      fontSize: size,
      lineHeight: 0.95,
      fontWeight: 900,
      letterSpacing: 0,
      maxWidth: 1320,
      textWrap: 'balance',
    }}
  >
    {children}
  </div>
);

const Body: React.FC<{children: React.ReactNode; width?: number}> = ({
  children,
  width = 980,
}) => (
  <div
    style={{
      color: colors.muted,
      fontSize: 36,
      lineHeight: 1.25,
      maxWidth: width,
      textWrap: 'balance',
    }}
  >
    {children}
  </div>
);

const CodeCard: React.FC<{
  label: string;
  code: string[];
  color: string;
  frame: number;
}> = ({label, code, color, frame}) => {
  const visible = Math.floor(interpolate(frame, [0, 52], [1, code.length], clamp));
  return (
    <div
      style={{
        width: 570,
        borderRadius: 22,
        border: `1px solid ${color}66`,
        background: 'rgba(12, 18, 25, 0.9)',
        boxShadow: `0 24px 70px ${color}18`,
        padding: 28,
      }}
    >
      <Kicker color={color}>{label}</Kicker>
      <pre
        style={{
          margin: '22px 0 0',
          color: '#e9f2ff',
          fontFamily: '"SFMono-Regular", "Roboto Mono", Consolas, monospace',
          fontSize: 27,
          lineHeight: 1.45,
          whiteSpace: 'pre-wrap',
        }}
      >
        {code.slice(0, visible).join('\n')}
      </pre>
    </div>
  );
};

const ScreenshotCard: React.FC<{
  shot: Shot;
  frame: number;
  large?: boolean;
}> = ({shot, frame, large = false}) => {
  const scale = interpolate(frame, [0, 80], [0.94, 1], {...clamp, easing: ease});
  const shine = interpolate(frame % 120, [0, 60, 120], [-60, 120, 260]);
  return (
    <div
      style={{
        position: 'relative',
        width: large ? 1160 : 760,
        borderRadius: 28,
        border: `1px solid ${shot.accent}70`,
        background: colors.panel,
        padding: 18,
        boxShadow: `0 35px 120px ${shot.accent}22`,
        transform: `scale(${scale})`,
        overflow: 'hidden',
      }}
    >
      <Img
        src={staticFile(shot.image)}
        style={{
          display: 'block',
          width: '100%',
          borderRadius: 18,
          imageRendering: 'pixelated',
        }}
      />
      <div
        style={{
          position: 'absolute',
          top: 0,
          bottom: 0,
          left: `${shine}%`,
          width: 160,
          rotate: '18deg',
          background: 'linear-gradient(90deg, transparent, rgba(255,255,255,0.24), transparent)',
        }}
      />
    </div>
  );
};

const Stat: React.FC<{value: string; label: string; color: string; frame: number}> = ({
  value,
  label,
  color,
  frame,
}) => (
  <div
    style={{
      padding: '30px 34px',
      borderRadius: 22,
      border: `1px solid ${color}55`,
      background: 'rgba(255,255,255,0.055)',
      transform: `translateY(${enterY(frame, 0, 24)}px)`,
    }}
  >
    <div style={{fontSize: 64, fontWeight: 900, color}}>{value}</div>
    <div style={{fontSize: 25, color: colors.muted, marginTop: 8}}>{label}</div>
  </div>
);

const HookScene = (frame: number, duration: number) => (
  <Shell frame={frame} duration={duration} accent={colors.teal}>
    <div
      style={{
        position: 'absolute',
        left: 150,
        top: 210,
        display: 'flex',
        flexDirection: 'column',
        gap: 28,
        transform: `translateY(${enterY(frame, 0, 60)}px)`,
        opacity: fade(frame, 0, 24),
      }}
    >
      <Kicker>Show HN ready</Kicker>
      <BigTitle size={138}>Machine Lab</BigTitle>
      <Body width={1040}>Portable C courseware for machine-facing programming.</Body>
    </div>
    <div
      style={{
        position: 'absolute',
        right: 150,
        top: 210,
        display: 'flex',
        flexDirection: 'column',
        gap: 18,
      }}
    >
      {['setup', 'make', 'test', 'trace', 'ship'].map((item, i) => (
        <div
          key={item}
          style={{
            opacity: fade(frame, 16 + i * 10, 32 + i * 10),
            transform: `translateX(${interpolate(frame, [16 + i * 10, 34 + i * 10], [80, 0], {...clamp, easing: ease})}px)`,
            color: i % 2 ? colors.yellow : colors.teal,
            fontFamily: '"SFMono-Regular", "Roboto Mono", Consolas, monospace',
            fontSize: 54,
            fontWeight: 800,
          }}
        >
          {`machinelab ${item}`}
        </div>
      ))}
    </div>
  </Shell>
);

const PromiseScene = (frame: number, duration: number) => (
  <Shell frame={frame} duration={duration} accent={colors.coral}>
    <div
      style={{
        position: 'absolute',
        inset: '150px 150px',
        display: 'grid',
        gridTemplateColumns: '1.05fr 0.95fr',
        gap: 70,
        alignItems: 'center',
      }}
    >
      <div style={{display: 'flex', flexDirection: 'column', gap: 30}}>
        <Kicker color={colors.coral}>The course promise</Kicker>
        <BigTitle>Old PC ideas. Modern classroom setup.</BigTitle>
        <Body>
          Keep the device contracts. Remove the VM ceremony. Let students build
          and test on the machine they already have.
        </Body>
      </div>
      <div style={{display: 'grid', gap: 22}}>
        {[
          ['No first-week VM trap', colors.coral],
          ['Deterministic tests', colors.yellow],
          ['Screenshots, WAVs, traces', colors.blue],
          ['Final projects with real loops', colors.green],
        ].map(([text, color], i) => (
          <div
            key={text}
            style={{
              padding: '28px 34px',
              borderRadius: 24,
              background: 'rgba(255,255,255,0.07)',
              border: `1px solid ${color}55`,
              fontSize: 34,
              fontWeight: 800,
              opacity: fade(frame, 10 + i * 8, 24 + i * 8),
              transform: `translateY(${enterY(frame, 10 + i * 8, 34)}px)`,
            }}
          >
            {text}
          </div>
        ))}
      </div>
    </div>
  </Shell>
);

const LabsScene = (frame: number, duration: number) => (
  <Shell frame={frame} duration={duration} accent={colors.yellow}>
    <div
      style={{
        position: 'absolute',
        left: 150,
        top: 130,
        display: 'flex',
        flexDirection: 'column',
        gap: 24,
      }}
    >
      <Kicker color={colors.yellow}>What students touch</Kicker>
      <BigTitle size={92}>Seven labs. One mental model.</BigTitle>
    </div>
    <div
      style={{
        position: 'absolute',
        left: 150,
        right: 150,
        bottom: 155,
        display: 'grid',
        gridTemplateColumns: 'repeat(7, 1fr)',
        gap: 18,
      }}
    >
      {labs.map(([name, detail, color], i) => (
        <div
          key={name}
          style={{
            height: 300,
            borderRadius: 28,
            padding: 24,
            border: `1px solid ${color}70`,
            background: `linear-gradient(180deg, ${color}22, rgba(255,255,255,0.055))`,
            opacity: fade(frame, i * 6, i * 6 + 18),
            transform: `translateY(${enterY(frame, i * 6, 60)}px)`,
            display: 'flex',
            flexDirection: 'column',
            justifyContent: 'space-between',
          }}
        >
          <div style={{fontSize: 44, fontWeight: 900, color}}>{name}</div>
          <div style={{fontSize: 25, color: colors.ink, lineHeight: 1.15}}>{detail}</div>
        </div>
      ))}
    </div>
  </Shell>
);

const CodeScene = (frame: number, duration: number) => (
  <Shell frame={frame} duration={duration} accent={colors.blue}>
    <div style={{position: 'absolute', left: 150, top: 115}}>
      <Kicker color={colors.blue}>Actual low-level habits</Kicker>
      <BigTitle size={86}>Status before data. State before rendering.</BigTitle>
    </div>
    <div
      style={{
        position: 'absolute',
        left: 150,
        right: 150,
        bottom: 125,
        display: 'flex',
        gap: 28,
        justifyContent: 'space-between',
      }}
    >
      {snippets.map((snippet, i) => (
        <div
          key={snippet.label}
          style={{
            opacity: fade(frame, i * 16, i * 16 + 20),
            transform: `translateY(${enterY(frame, i * 16, 42)}px)`,
          }}
        >
          <CodeCard {...snippet} frame={Math.max(0, frame - i * 16)} />
        </div>
      ))}
    </div>
  </Shell>
);

const CliScene = (frame: number, duration: number) => (
  <Shell frame={frame} duration={duration} accent={colors.green}>
    <div
      style={{
        position: 'absolute',
        left: 150,
        top: 142,
        width: 820,
        display: 'flex',
        flexDirection: 'column',
        gap: 26,
      }}
    >
      <Kicker color={colors.green}>Start in minutes</Kicker>
      <BigTitle size={100}>Generate a student workspace.</BigTitle>
      <Body>Students see labs, tests, and Makefiles. Maintainers keep the runtime internals.</Body>
    </div>
    <div
      style={{
        position: 'absolute',
        right: 150,
        top: 160,
        width: 760,
        borderRadius: 28,
        background: '#090d12',
        border: `1px solid ${colors.green}55`,
        boxShadow: `0 30px 100px ${colors.green}20`,
        padding: 34,
        fontFamily: '"SFMono-Regular", "Roboto Mono", Consolas, monospace',
        fontSize: 33,
        lineHeight: 1.5,
      }}
    >
      {[
        '$ machinelab setup student',
        '$ cd student',
        '$ make',
        '$ machinelab test rtc',
        'rtc_read_date FAILED',
        'good: the lab has gaps',
      ].map((line, i) => (
        <div
          key={line}
          style={{
            color: line.includes('FAILED') ? colors.coral : line.startsWith('good') ? colors.yellow : '#d7f8ee',
            opacity: fade(frame, 8 + i * 12, 18 + i * 12),
          }}
        >
          {line}
        </div>
      ))}
    </div>
  </Shell>
);

const ScreenshotScene = (shot: Shot) => (frame: number, duration: number) => (
  <Shell frame={frame} duration={duration} accent={shot.accent}>
    <div
      style={{
        position: 'absolute',
        left: 125,
        right: 125,
        top: 120,
        bottom: 110,
        display: 'grid',
        gridTemplateColumns: '0.86fr 1.14fr',
        gap: 72,
        alignItems: 'center',
      }}
    >
      <div style={{display: 'flex', flexDirection: 'column', gap: 28}}>
        <Kicker color={shot.accent}>Project seed</Kicker>
        <BigTitle size={96}>{shot.title}</BigTitle>
        <Body>{shot.subtitle}</Body>
      </div>
      <ScreenshotCard shot={shot} frame={frame} large />
    </div>
  </Shell>
);

const MontageScene = (frame: number, duration: number) => (
  <Shell frame={frame} duration={duration} accent={colors.violet}>
    <div style={{position: 'absolute', left: 150, top: 120}}>
      <Kicker color={colors.violet}>Final project energy</Kicker>
      <BigTitle size={88}>Games, editors, trackers, protocols.</BigTitle>
    </div>
    <div
      style={{
        position: 'absolute',
        left: 150,
        right: 150,
        bottom: 115,
        display: 'grid',
        gridTemplateColumns: 'repeat(3, 1fr)',
        gap: 28,
      }}
    >
      {[screenshots[2], screenshots[3], screenshots[4]].map((shot, i) => (
        <div
          key={shot.title}
          style={{
            opacity: fade(frame, 10 + i * 14, 28 + i * 14),
            transform: `translateY(${enterY(frame, 10 + i * 14, 54)}px) rotate(${i === 1 ? '-2deg' : i === 2 ? '2deg' : '0deg'})`,
          }}
        >
          <ScreenshotCard shot={shot} frame={frame - i * 14} />
          <div style={{fontSize: 30, fontWeight: 900, marginTop: 18, color: shot.accent}}>
            {shot.title}
          </div>
        </div>
      ))}
    </div>
  </Shell>
);

const ArchitectureScene = (frame: number, duration: number) => (
  <Shell frame={frame} duration={duration} accent={colors.teal}>
    <div
      style={{
        position: 'absolute',
        left: 150,
        top: 132,
        display: 'flex',
        flexDirection: 'column',
        gap: 24,
      }}
    >
      <Kicker>What the labs teach</Kicker>
      <BigTitle size={90}>A clean reactive architecture.</BigTitle>
    </div>
    <div
      style={{
        position: 'absolute',
        left: 190,
        right: 190,
        bottom: 150,
        display: 'grid',
        gridTemplateColumns: 'repeat(4, 1fr)',
        gap: 28,
      }}
    >
      {[
        ['Device helpers', 'scancodes, packets, ticks, pixels', colors.blue],
        ['Application state', 'menus, documents, songs, entities', colors.yellow],
        ['Renderer', 'draw current state into frames', colors.coral],
        ['Replay/test', 'deterministic timelines', colors.green],
      ].map(([title, detail, color], i) => (
        <div
          key={title}
          style={{
            minHeight: 330,
            borderRadius: 28,
            padding: 34,
            background: 'rgba(255,255,255,0.07)',
            border: `1px solid ${color}66`,
            opacity: fade(frame, i * 10, i * 10 + 20),
            transform: `translateY(${enterY(frame, i * 10, 50)}px)`,
          }}
        >
          <div style={{fontSize: 42, fontWeight: 900, color}}>{title}</div>
          <div style={{fontSize: 28, lineHeight: 1.3, color: colors.muted, marginTop: 22}}>
            {detail}
          </div>
        </div>
      ))}
    </div>
  </Shell>
);

const AdoptionScene = (frame: number, duration: number) => (
  <Shell frame={frame} duration={duration} accent={colors.yellow}>
    <div
      style={{
        position: 'absolute',
        left: 150,
        right: 150,
        top: 150,
        display: 'grid',
        gridTemplateColumns: '1fr 1fr',
        gap: 80,
        alignItems: 'center',
      }}
    >
      <div style={{display: 'flex', flexDirection: 'column', gap: 26}}>
        <Kicker color={colors.yellow}>Adoptable courseware</Kicker>
        <BigTitle size={92}>Free for individuals. Practical for universities.</BigTitle>
        <Body>Use it as a workshop, a module, or a 6 ECTS lab-project course.</Body>
      </div>
      <div style={{display: 'grid', gridTemplateColumns: 'repeat(2, 1fr)', gap: 22}}>
        <Stat value="6" label="ECTS course shape" color={colors.yellow} frame={frame} />
        <Stat value="52" label="contact hours" color={colors.teal} frame={frame - 8} />
        <Stat value="162" label="total hours" color={colors.coral} frame={frame - 16} />
        <Stat value="MIT/CC" label="reuse-friendly licenses" color={colors.green} frame={frame - 24} />
      </div>
    </div>
  </Shell>
);

const InstructorScene = (frame: number, duration: number) => (
  <Shell frame={frame} duration={duration} accent={colors.blue}>
    <div style={{position: 'absolute', left: 150, top: 125}}>
      <Kicker color={colors.blue}>Instructor kit</Kicker>
      <BigTitle size={90}>Labs, guides, rubrics, releases.</BigTitle>
    </div>
    <div
      style={{
        position: 'absolute',
        left: 150,
        right: 150,
        bottom: 150,
        display: 'grid',
        gridTemplateColumns: 'repeat(4, 1fr)',
        gap: 24,
      }}
    >
      {[
        'public lab guides',
        'generated workspaces',
        'deterministic checks',
        'project rubric',
        'Cloudflare docs',
        'multi-platform releases',
        'demo artifacts',
        'developer docs',
      ].map((item, i) => (
        <div
          key={item}
          style={{
            height: 120,
            borderRadius: 22,
            padding: 24,
            background: 'rgba(255,255,255,0.07)',
            border: `1px solid ${i % 2 ? colors.blue : colors.teal}55`,
            fontSize: 28,
            fontWeight: 800,
            display: 'flex',
            alignItems: 'center',
            opacity: fade(frame, i * 5, i * 5 + 14),
          }}
        >
          {item}
        </div>
      ))}
    </div>
  </Shell>
);

const StudentScene = (frame: number, duration: number) => (
  <Shell frame={frame} duration={duration} accent={colors.coral}>
    <div
      style={{
        position: 'absolute',
        inset: '150px 150px',
        display: 'grid',
        gridTemplateColumns: '1fr 1fr',
        gap: 70,
        alignItems: 'center',
      }}
    >
      <div
        style={{
          borderRadius: 30,
          background: '#0a0f15',
          border: `1px solid ${colors.coral}55`,
          padding: 36,
          fontFamily: '"SFMono-Regular", "Roboto Mono", Consolas, monospace',
          fontSize: 30,
          lineHeight: 1.55,
        }}
      >
        {[
          'rtc_read_date FAILED',
          'bit_mask simple FAILED',
          'timer_ticks passed',
          'kbd scancode passed',
          'graphics clip passed',
          'audio wav written',
        ].map((line, i) => (
          <div
            key={line}
            style={{
              opacity: fade(frame, i * 10, i * 10 + 14),
              color: line.includes('FAILED') ? colors.coral : colors.green,
            }}
          >
            {line}
          </div>
        ))}
      </div>
      <div style={{display: 'flex', flexDirection: 'column', gap: 28}}>
        <Kicker color={colors.coral}>Student loop</Kicker>
        <BigTitle size={94}>Fail. Inspect. Implement. Pass.</BigTitle>
        <Body>Tests are not the end. They are feedback while students learn the device protocol.</Body>
      </div>
    </div>
  </Shell>
);

const ArtifactsScene = (frame: number, duration: number) => (
  <Shell frame={frame} duration={duration} accent={colors.green}>
    <div style={{position: 'absolute', left: 150, top: 130}}>
      <Kicker color={colors.green}>Proof you can see and hear</Kicker>
      <BigTitle size={86}>Not just terminal output.</BigTitle>
    </div>
    <div
      style={{
        position: 'absolute',
        left: 150,
        right: 150,
        bottom: 150,
        display: 'grid',
        gridTemplateColumns: 'repeat(5, 1fr)',
        gap: 20,
      }}
    >
      {[
        ['screenshots', colors.blue],
        ['WAVs', colors.violet],
        ['traces', colors.yellow],
        ['videos', colors.coral],
        ['bundles', colors.green],
      ].map(([label, color], i) => (
        <div
          key={label}
          style={{
            height: 260,
            borderRadius: 140,
            border: `1px solid ${color}66`,
            background: `radial-gradient(circle at 50% 35%, ${color}44, rgba(255,255,255,0.05))`,
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            fontSize: 35,
            fontWeight: 900,
            opacity: fade(frame, 8 + i * 8, 22 + i * 8),
            transform: `translateY(${enterY(frame, 8 + i * 8, 46)}px)`,
          }}
        >
          {label}
        </div>
      ))}
    </div>
  </Shell>
);

const ClosingScene = (frame: number, duration: number) => (
  <Shell frame={frame} duration={duration} accent={colors.teal}>
    <div
      style={{
        position: 'absolute',
        inset: 0,
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        justifyContent: 'center',
        gap: 36,
        textAlign: 'center',
      }}
    >
      <Kicker>Machine Lab</Kicker>
      <BigTitle size={120}>Teach the machine-facing parts.</BigTitle>
      <Body width={1120}>Free courseware for C labs, device-shaped systems programming, and final projects students can demo.</Body>
      <div
        style={{
          marginTop: 26,
          fontFamily: '"SFMono-Regular", "Roboto Mono", Consolas, monospace',
          fontSize: 34,
          color: colors.yellow,
          opacity: fade(frame, 50, 80),
        }}
      >
        machine-lab-docs.pages.dev
      </div>
    </div>
  </Shell>
);

const scenes: Scene[] = [
  {start: 0, duration: 150, render: (f) => HookScene(f, 150)},
  {start: 150, duration: 180, render: (f) => PromiseScene(f, 180)},
  {start: 330, duration: 180, render: (f) => LabsScene(f, 180)},
  {start: 510, duration: 180, render: (f) => CodeScene(f, 180)},
  {start: 690, duration: 210, render: (f) => CliScene(f, 210)},
  {start: 900, duration: 210, render: (f) => ScreenshotScene(screenshots[0])(f, 210)},
  {start: 1110, duration: 210, render: (f) => ScreenshotScene(screenshots[1])(f, 210)},
  {start: 1320, duration: 210, render: (f) => MontageScene(f, 210)},
  {start: 1530, duration: 180, render: (f) => ArchitectureScene(f, 180)},
  {start: 1710, duration: 180, render: (f) => AdoptionScene(f, 180)},
  {start: 1890, duration: 210, render: (f) => InstructorScene(f, 210)},
  {start: 2100, duration: 210, render: (f) => StudentScene(f, 210)},
  {start: 2310, duration: 210, render: (f) => ArtifactsScene(f, 210)},
  {start: 2520, duration: 180, render: (f) => ClosingScene(f, 180)},
];

export const MachineLabAnnouncement: React.FC = () => {
  const frame = useCurrentFrame();
  useVideoConfig();

  return (
    <AbsoluteFill style={{background: colors.bg}}>
      {scenes.map((scene, index) => {
        if (frame < scene.start || frame >= scene.start + scene.duration) {
          return null;
        }
        return (
          <React.Fragment key={scene.start}>
            {scene.render(frame - scene.start, index)}
          </React.Fragment>
        );
      })}
    </AbsoluteFill>
  );
};
