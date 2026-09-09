package org.isomorphisms.ib.material3

import android.content.res.AssetManager
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.weight
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.selection.SelectionContainer
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.ListItem
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import java.io.IOException

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val articles = loadPensieveArticles(assets)
        setContent {
            MaterialTheme {
                ArxivViewport(articles)
            }
        }
    }
}

private data class PensieveArticle(
    val arxivId: String,
    val title: String,
    val body: String?,
    val textSource: String?,
)

private fun readAssetOrNull(assets: AssetManager, path: String): String? =
    try {
        assets.open(path).bufferedReader().use { it.readText() }
    } catch (_: IOException) {
        null
    }

private fun loadPensieveArticles(assets: AssetManager): List<PensieveArticle> {
    val root = "pensieve/arxiv"
    val identifiers = assets.list(root)
        ?.filter { it.isNotBlank() }
        ?.sorted()
        .orEmpty()

    return identifiers.map { identifier ->
        val itemRoot = "$root/$identifier"
        val title = readAssetOrNull(assets, "$itemRoot/title")
            ?.trim()
            ?.takeIf { it.isNotEmpty() }
            ?: "arXiv $identifier"

        val textCandidates = listOf(
            "text/from-pdf.txt",
            "text/from-html.txt",
            "text/from-abstract.txt",
        )

        var body: String? = null
        var textSource: String? = null
        for (candidate in textCandidates) {
            val text = readAssetOrNull(assets, "$itemRoot/$candidate")
                ?.trim()
                ?.takeIf { it.isNotEmpty() }
            if (text != null) {
                body = text
                textSource = candidate
                break
            }
        }

        PensieveArticle(
            arxivId = identifier,
            title = title,
            body = body,
            textSource = textSource,
        )
    }
}

@Composable
private fun ArxivViewport(articles: List<PensieveArticle>) {
    val articlesById = remember(articles) { articles.associateBy { it.arxivId } }

    var selectedId by remember(articles) {
        mutableStateOf(articles.firstOrNull()?.arxivId)
    }
    var recentOrder by remember(articles) {
        mutableStateOf(articles.map { it.arxivId })
    }
    var pinnedOrder by remember(articles) {
        mutableStateOf(emptyList<String>())
    }

    fun selectArticle(article: PensieveArticle) {
        selectedId = article.arxivId
        recentOrder = listOf(article.arxivId) + recentOrder.filterNot {
            it == article.arxivId
        }
    }

    fun togglePinned(article: PensieveArticle) {
        pinnedOrder = if (article.arxivId in pinnedOrder) {
            pinnedOrder.filterNot { it == article.arxivId }
        } else {
            listOf(article.arxivId) + pinnedOrder
        }
    }

    val pinnedArticles = pinnedOrder.mapNotNull(articlesById::get)
    val pinnedIds = pinnedOrder.toSet()
    val recentArticles = recentOrder
        .asSequence()
        .filterNot(pinnedIds::contains)
        .mapNotNull(articlesById::get)
        .toList()
    val selected = selectedId?.let(articlesById::get)

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("IB") },
            )
        },
    ) { scaffoldPadding ->
        Row(
            modifier = Modifier
                .fillMaxSize()
                .padding(scaffoldPadding),
        ) {
            Column(
                modifier = Modifier
                    .width(320.dp)
                    .fillMaxHeight(),
            ) {
                Text(
                    text = "Pensieve · ${articles.size} arXiv entries",
                    style = MaterialTheme.typography.labelLarge,
                    modifier = Modifier.padding(horizontal = 16.dp, vertical = 10.dp),
                )

                LazyColumn(
                    modifier = Modifier
                        .fillMaxWidth()
                        .weight(1f),
                ) {
                    if (pinnedArticles.isNotEmpty()) {
                        item(key = "pinned-heading") {
                            RailHeading("Pinned")
                        }
                        items(
                            items = pinnedArticles,
                            key = { article -> article.arxivId },
                        ) { article ->
                            ArticleRailItem(
                                article = article,
                                pinned = true,
                                onSelect = { selectArticle(article) },
                                onTogglePinned = { togglePinned(article) },
                            )
                        }
                    }

                    item(key = "recent-heading") {
                        RailHeading("Recent")
                    }
                    items(
                        items = recentArticles,
                        key = { article -> article.arxivId },
                    ) { article ->
                        ArticleRailItem(
                            article = article,
                            pinned = false,
                            onSelect = { selectArticle(article) },
                            onTogglePinned = { togglePinned(article) },
                        )
                    }
                }
            }

            Surface(
                modifier = Modifier
                    .fillMaxHeight()
                    .weight(1f)
                    .padding(12.dp),
                shape = MaterialTheme.shapes.large,
                tonalElevation = 1.dp,
            ) {
                SelectionContainer {
                    Column(
                        modifier = Modifier
                            .fillMaxSize()
                            .verticalScroll(rememberScrollState())
                            .padding(18.dp),
                    ) {
                        if (selected == null) {
                            Text(
                                text = "The Pensieve snapshot is empty.",
                                style = MaterialTheme.typography.bodyLarge,
                            )
                        } else {
                            Text(
                                text = selected.title,
                                style = MaterialTheme.typography.headlineSmall,
                            )
                            Text(
                                text = "arXiv ${selected.arxivId}",
                                style = MaterialTheme.typography.bodyMedium,
                            )
                            Text(
                                text = selected.textSource?.let {
                                    "Pensieve · arxiv/${selected.arxivId}/$it"
                                } ?: "Pensieve · arxiv/${selected.arxivId}",
                                style = MaterialTheme.typography.labelMedium,
                            )
                            Spacer(modifier = Modifier.height(18.dp))
                            Text(
                                text = selected.body
                                    ?: "No text representation is present in this frozen Pensieve snapshot.",
                                style = MaterialTheme.typography.bodyLarge,
                            )
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun RailHeading(text: String) {
    Text(
        text = text,
        style = MaterialTheme.typography.labelMedium,
        color = MaterialTheme.colorScheme.onSurfaceVariant,
        modifier = Modifier.padding(horizontal = 16.dp, vertical = 8.dp),
    )
}

@Composable
private fun ArticleRailItem(
    article: PensieveArticle,
    pinned: Boolean,
    onSelect: () -> Unit,
    onTogglePinned: () -> Unit,
) {
    ListItem(
        headlineContent = { Text(article.title) },
        supportingContent = {
            Text("Pensieve · arxiv/${article.arxivId}")
        },
        trailingContent = {
            TextButton(onClick = onTogglePinned) {
                Text(if (pinned) "Unpin" else "Pin")
            }
        },
        modifier = Modifier.clickable(onClick = onSelect),
    )
    HorizontalDivider()
}
