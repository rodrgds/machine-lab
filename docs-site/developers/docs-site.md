# Docs Site

The public website is built with VitePress and deployed to Cloudflare Pages.

Production URL: <https://machine-lab-docs.pages.dev/>

## Commands

```sh
npm install
npm run docs:dev
npm run docs:check
npm run deploy:docs
```

`docs:check` builds the site and runs `scripts/check_docs_site.sh`.

## Source Layout

| Path | Purpose |
| --- | --- |
| `docs-site/index.md` | public landing page |
| `docs-site/labs/` | student lab guide chapters |
| `docs-site/students/` | student workflow |
| `docs-site/instructors/` | adoption, syllabus, rubric |
| `docs-site/developers/` | maintainer documentation |
| `docs-site/examples/` | demos and project seeds |
| `docs-site/.vitepress/config.mts` | navigation, sidebars, metadata |
| `docs-site/.vitepress/theme/` | site styling |

## Assets

Images and videos are source-controlled under `docs/assets/`. The site build
copies them into `docs-site/public/assets/`:

```sh
npm run docs:assets
```

Do not edit generated files under `docs-site/public/assets/` directly.

## Cloudflare Pages

The project is `machine-lab-docs`.

`wrangler.toml` sets:

```toml
name = "machine-lab-docs"
pages_build_output_dir = "docs-site/.vitepress/dist"
```

Push-to-deploy is configured in `.github/workflows/docs.yml`. It needs:

- `CLOUDFLARE_ACCOUNT_ID`
- `CLOUDFLARE_API_TOKEN`

The account ID secret is already set. The API token must be created with Pages
deployment permission and added as a repository secret.
